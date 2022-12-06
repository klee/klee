#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Expr/SymbolicSource.h"

#include "klee/Solver/AddressGenerator.h"
#include "klee/Solver/ConcretizationManager.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"

#include "llvm/Support/Casting.h"

#include <algorithm>
#include <vector>

namespace klee {

class ConcretizingSolver : public SolverImpl {
private:
  Solver *solver;
  ConcretizationManager *concretizationManager;
  AddressGenerator *addressGenerator;

public:
  ConcretizingSolver(Solver *_solver, ConcretizationManager *_cm,
                     AddressGenerator *_ag)
      : solver(_solver), concretizationManager(_cm), addressGenerator(_ag) {}

  ~ConcretizingSolver() { delete solver; }

  bool computeTruth(const Query &, bool &isValid);
  bool computeValidity(const Query &, Solver::Validity &result);
  bool computeValidity(const Query &query, ref<SolverResponse> &queryResult,
                       ref<SolverResponse> &negatedQueryResult);

  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid);
  bool check(const Query &query, ref<SolverResponse> &result);

  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);

private:
  bool assertConcretization(const Query &query, const Assignment &assign) const;
  bool relaxSymcreteConstraints(const Query &query,
                                ref<SolverResponse> &result);
  Query constructConcretizedQuery(const Query &, const Assignment &);
};

Query ConcretizingSolver::constructConcretizedQuery(const Query &query,
                                                    const Assignment &assign) {
  ConstraintSet constraints = assign.createConstraintsFromAssignment();
  for (auto e : query.constraints) {
    constraints.push_back(e);
  }
  return Query(constraints, query.expr);
}

bool ConcretizingSolver::assertConcretization(const Query &query,
                                              const Assignment &assign) const {
  for (const Array *symcreteArray : query.gatherSymcreteArrays()) {
    if (!assign.bindings.count(symcreteArray)) {
      return false;
    }
  }
  return true;
}

bool ConcretizingSolver::relaxSymcreteConstraints(const Query &query,
                                                  ref<SolverResponse> &result) {
  // Get initial symcrete solution. We will try to relax
  // them in order to achieve `mayBeTrue` solution.
  Assignment assignment = query.constraints.getConcretization();

  std::vector<const Array *> brokenSymcreteArrays;
  std::vector<ref<Expr>> brokenSizes;

  bool wereConcretizationsRemoved = true;
  while (wereConcretizationsRemoved) {
    wereConcretizationsRemoved = false;
    if (!solver->impl->check(constructConcretizedQuery(query, assignment),
                             result)) {
      return false;
    }
    // No unsat cores were found for the query, so we can try
    // to find new solution.
    if (isa<InvalidResponse>(result)) {
      break;
    }

    ValidityCore validityCore;
    assert(result->tryGetValidityCore(validityCore));
    std::vector<const Array *> currentlyBrokenSymcreteArrays =
        Query(ConstraintSet(validityCore.constraints), validityCore.expr)
            .gatherSymcreteArrays();

    for (const Array *brokenArray : currentlyBrokenSymcreteArrays) {
      if (!assignment.bindings.count(brokenArray)) {
        continue;
      }

      if (ref<SymbolicSizeSource> sizeSource =
              dyn_cast<SymbolicSizeSource>(brokenArray->source)) {
        // Remove size concretization
        assignment.bindings.erase(brokenArray);
        brokenSymcreteArrays.push_back(brokenArray);

        // Remove address concretization
        assignment.bindings.erase(sizeSource->linkedArray);
        brokenSymcreteArrays.push_back(sizeSource->linkedArray);

        wereConcretizationsRemoved = true;
        brokenSizes.push_back(Expr::createTempRead(brokenArray, Expr::Int64));
        // Add symbolic size to the sum that should be minimized.
      }
    }
  }

  if (isa<ValidResponse>(result)) {
    return true;
  }

  Query concretizedNegatedQuery =
      constructConcretizedQuery(query.negateExpr(), assignment);

  ConstraintSet queryConstraints = concretizedNegatedQuery.constraints;
  ConstraintManager queryConstraintsManager(queryConstraints);
  queryConstraintsManager.addConstraint(concretizedNegatedQuery.expr);

  assert(!brokenSizes.empty());
  ref<Expr> symbolicSizesSum = createNonOverflowingSumExpr(brokenSizes);
  symbolicSizesSum =
      ConstraintManager::simplifyExpr(query.constraints, symbolicSizesSum);

  ref<ConstantExpr> minimalValueOfSum;
  if (!solver->impl->computeMinimalUnsignedValue(
          Query(queryConstraints, symbolicSizesSum), minimalValueOfSum)) {
    return false;
  }

  std::vector<SparseStorage<unsigned char>> brokenSymcretesValues;
  bool hasSolution = false;
  if (!solver->impl->computeInitialValues(
          Query(queryConstraints,
                EqExpr::create(symbolicSizesSum, minimalValueOfSum))
              .negateExpr(),
          brokenSymcreteArrays, brokenSymcretesValues, hasSolution)) {
    return false;
  }
  assert(hasSolution && "Symcretes values should have concretization after "
                        "computeInitialValues() query.");

  for (unsigned idx = 0; idx < brokenSymcreteArrays.size(); ++idx) {
    if (isa<SymbolicSizeSource>(brokenSymcreteArrays[idx]->source)) {
      assignment.bindings[brokenSymcreteArrays[idx]] =
          brokenSymcretesValues[idx];
      // Receive address array linked with this size array to request address
      // concretization.
      if (ref<SymbolicAllocationSource> allocSource =
              dyn_cast_or_null<SymbolicAllocationSource>(
                  brokenSymcreteArrays[idx]->source)) {
        const Array *dependentAddressArray = allocSource->linkedArray;

        uint64_t newSize =
            cast<ConstantExpr>(assignment.evaluate(Expr::createTempRead(
                                   brokenSymcreteArrays[idx], 64)))
                ->getZExtValue();

        void *address =
            addressGenerator->allocate(dependentAddressArray, newSize);
        assert(address);
        assignment.bindings[dependentAddressArray] =
            sparseBytesFromValue(address);
      }
    }
  }

  if (!solver->impl->check(constructConcretizedQuery(query, assignment),
                           result)) {
    return false;
  }

  return true;
}

bool ConcretizingSolver::computeValidity(const Query &query,
                                         Solver::Validity &result) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValidity(query, result);
  }
  ref<SolverResponse> queryResult, negatedQueryResult;
  if (!computeValidity(query, queryResult, negatedQueryResult)) {
    return false;
  }
  if (isa<ValidResponse>(queryResult)) {
    result = Solver::True;
  } else if (isa<ValidResponse>(negatedQueryResult)) {
    result = Solver::False;
  } else {
    result = Solver::Unknown;
  }
  return true;
}

bool ConcretizingSolver::computeValidity(
    const Query &query, ref<SolverResponse> &queryResult,
    ref<SolverResponse> &negatedQueryResult) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValidity(query, queryResult,
                                         negatedQueryResult);
  }
  auto assign = query.constraints.getConcretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  auto concretizedQuery = constructConcretizedQuery(query, assign);
  if (!solver->impl->computeValidity(concretizedQuery, queryResult,
                                     negatedQueryResult)) {
    return false;
  }

  std::vector<SparseStorage<unsigned char>> queryResultValues,
      negatedQueryResultValues;

  std::vector<const Array *> objects = assign.keys();

  assert(isa<InvalidResponse>(queryResult) ||
         isa<InvalidResponse>(negatedQueryResult));

  // *No more than one* of queryResult and negatedQueryResult is possible,
  // i.e. `mustBeTrue` with values from `assign`.
  // Take one which is `mustBeTrue` with symcretes from `assign`
  // and try to relax them to `mayBeFalse`. This solution should be
  // appropriate for the remain branch.

  if (isa<ValidResponse>(queryResult)) {
    concretizationManager->add(
        query,
        cast<InvalidResponse>(negatedQueryResult)->initialValuesFor(objects));
    if (!relaxSymcreteConstraints(query, queryResult)) {
      return false;
    }
    if (ref<InvalidResponse> queryInvalidResponse =
            dyn_cast<InvalidResponse>(queryResult)) {
      concretizationManager->add(
          query.negateExpr(), queryInvalidResponse->initialValuesFor(objects));
    }
  } else if (isa<ValidResponse>(negatedQueryResult)) {
    concretizationManager->add(
        query.negateExpr(),
        cast<InvalidResponse>(queryResult)->initialValuesFor(objects));
    if (!relaxSymcreteConstraints(query.negateExpr(), negatedQueryResult)) {
      return false;
    }

    if (ref<InvalidResponse> negatedQueryInvalidResponse =
            dyn_cast<InvalidResponse>(negatedQueryResult)) {
      concretizationManager->add(
          query, negatedQueryInvalidResponse->initialValuesFor(objects));
    }
  }

  return true;
}

bool ConcretizingSolver::check(const Query &query,
                               ref<SolverResponse> &result) {
  if (!query.containsSymcretes()) {
    return solver->impl->check(query, result);
  }
  auto assign = query.constraints.getConcretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  auto concretizedQuery = constructConcretizedQuery(query, assign);
  if (!solver->impl->check(concretizedQuery, result)) {
    return false;
  }

  if (isa<ValidResponse>(result)) {
    if (!relaxSymcreteConstraints(query, result)) {
      return false;
    }
  }

  if (ref<InvalidResponse> resultInvalidResponse =
          dyn_cast<InvalidResponse>(result)) {
    concretizationManager->add(
        query.negateExpr(),
        resultInvalidResponse->initialValuesFor(assign.keys()));
  }

  return true;
}

char *ConcretizingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

bool ConcretizingSolver::computeTruth(const Query &query, bool &isValid) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeTruth(query, isValid);
  }

  auto assign = query.constraints.getConcretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  if (ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(
          query.constraints.getConcretization().evaluate(query.expr))) {
    isValid = CE->isTrue();
  } else {
    auto concretizedQuery = constructConcretizedQuery(query, assign);

    if (!solver->impl->computeTruth(concretizedQuery, isValid)) {
      return false;
    }
  }

  // If constraints always evaluate to `mustBeTrue`, then relax
  // symcretes until remove all of them or query starts to evaluate
  // to `mayBeFalse`.

  ref<SolverResponse> result;
  if (isValid) {
    if (!relaxSymcreteConstraints(query, result)) {
      return false;
    }
    if (ref<InvalidResponse> resultInvalidResponse =
            dyn_cast<InvalidResponse>(result)) {
      assign = resultInvalidResponse->initialValuesFor(assign.keys());
      isValid = false;
    }
  }

  if (!isValid) {
    concretizationManager->add(query.negateExpr(), assign);
  }

  return true;
}

bool ConcretizingSolver::computeValidityCore(const Query &query,
                                             ValidityCore &validityCore,
                                             bool &isValid) {
  Assignment assign = query.constraints.getConcretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  Query concretizedQuery = constructConcretizedQuery(query, assign);

  if (ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(
          query.constraints.getConcretization().evaluate(query.expr))) {
    isValid = CE->isTrue();
  } else {
    if (!solver->impl->computeValidityCore(concretizedQuery, validityCore,
                                           isValid)) {
      return false;
    }
  }

  if (isValid) {
    ref<SolverResponse> result;
    if (!relaxSymcreteConstraints(query, result)) {
      return false;
    }
    /* Here we already have validity core from query above. */
    if (ref<InvalidResponse> resultInvalidResponse =
            dyn_cast<InvalidResponse>(result)) {
      assign = resultInvalidResponse->initialValuesFor(assign.keys());
    } else {
      assert(result->tryGetValidityCore(validityCore));
      isValid = false;
    }
  }

  if (!isValid) {
    validityCore = ValidityCore();
    concretizationManager->add(query.negateExpr(), assign);
  }

  return true;
}

bool ConcretizingSolver::computeValue(const Query &query, ref<Expr> &result) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValue(query, result);
  }

  Assignment assign = query.constraints.getConcretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  if (ref<ConstantExpr> expr = dyn_cast<ConstantExpr>(
          query.constraints.getConcretization().evaluate(query.expr))) {
    result = expr;
    return true;
  }
  auto concretizedQuery = constructConcretizedQuery(query, assign);
  return solver->impl->computeValue(concretizedQuery, result);
}

bool ConcretizingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeInitialValues(query, objects, values,
                                              hasSolution);
  }

  Assignment assign = query.constraints.getConcretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  auto concretizedQuery = constructConcretizedQuery(query, assign);
  if (!solver->impl->computeInitialValues(concretizedQuery, objects, values,
                                          hasSolution)) {
    return false;
  }

  if (!hasSolution) {
    ref<SolverResponse> result;
    if (!relaxSymcreteConstraints(query, result)) {
      return false;
    }
    /* Because relaxSymcreteConstraints response is `isValid`,
    and `isValid` == false iff solution for negation exists. */
    if (ref<InvalidResponse> resultInvalidResponse =
            dyn_cast<InvalidResponse>(result)) {
      hasSolution = true;
      assign = resultInvalidResponse->initialValuesFor(assign.keys());
      concretizationManager->add(query.negateExpr(), assign);
      values = std::vector<SparseStorage<unsigned char>>();
      return solver->impl->computeInitialValues(
          constructConcretizedQuery(query, assign), objects, values,
          hasSolution);
    }
  }

  return true;
}

// Redo later
SolverImpl::SolverRunStatus ConcretizingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

void ConcretizingSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->setCoreSolverTimeout(timeout);
}

Solver *createConcretizingSolver(Solver *s,
                                 ConcretizationManager *concretizationManager,
                                 AddressGenerator *addressGenerator) {
  return new Solver(
      new ConcretizingSolver(s, concretizationManager, addressGenerator));
}
} // namespace klee
