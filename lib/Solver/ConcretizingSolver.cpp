#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Expr/Symcrete.h"

#include "klee/Solver/AddressGenerator.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Solver/SolverUtil.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/Casting.h"
DISABLE_WARNING_POP

#include <algorithm>
#include <queue>
#include <vector>

namespace klee {

class ConcretizingSolver : public SolverImpl {
private:
  std::unique_ptr<Solver> solver;
  AddressGenerator *addressGenerator;

public:
  ConcretizingSolver(std::unique_ptr<Solver> _solver, AddressGenerator *_ag)
      : solver(std::move(_solver)), addressGenerator(_ag) {}

  ~ConcretizingSolver() = default;

  bool computeTruth(const Query &, bool &isValid);
  bool computeValidity(const Query &, PartialValidity &result);
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
  bool getBrokenArrays(const Query &query, const Assignment &diff,
                       ref<SolverResponse> &result,
                       std::vector<const Array *> &brokenArrays);
  bool relaxSymcreteConstraints(const Query &query,
                                ref<SolverResponse> &result);
  Query constructConcretizedQuery(const Query &, const Assignment &);
  Query getConcretizedVersion(const Query &);

private:
  void reverseConcretization(ValidityCore &validityCore,
                             const ExprHashMap<ref<Expr>> &reverse,
                             ref<Expr> reverseExpr);
  void reverseConcretization(ref<SolverResponse> &res,
                             const ExprHashMap<ref<Expr>> &reverse,
                             ref<Expr> reverseExpr);
};

Query ConcretizingSolver::constructConcretizedQuery(const Query &query,
                                                    const Assignment &assign) {
  ConstraintSet cs = query.constraints;
  ref<Expr> concretizedExpr = assign.evaluate(query.expr);
  return Query(cs.getConcretizedVersion(assign), concretizedExpr);
}

Query ConcretizingSolver::getConcretizedVersion(const Query &query) {
  ConstraintSet cs = query.constraints;
  ref<Expr> concretizedExpr = cs.concretization().evaluate(query.expr);
  return Query(cs.getConcretizedVersion(), concretizedExpr);
}

void ConcretizingSolver::reverseConcretization(
    ValidityCore &validityCore, const ExprHashMap<ref<Expr>> &reverse,
    ref<Expr> reverseExpr) {
  validityCore.expr = reverseExpr;
  for (auto e : reverse) {
    if (validityCore.constraints.find(e.second) !=
        validityCore.constraints.end()) {
      validityCore.constraints.erase(e.second);
      validityCore.constraints.insert(e.first);
    }
  }
}

void ConcretizingSolver::reverseConcretization(
    ref<SolverResponse> &res, const ExprHashMap<ref<Expr>> &reverse,
    ref<Expr> reverseExpr) {
  if (!isa<ValidResponse>(res)) {
    return;
  }
  ValidityCore validityCore;
  res->tryGetValidityCore(validityCore);
  reverseConcretization(validityCore, reverse, reverseExpr);
  res = new ValidResponse(validityCore);
}

bool ConcretizingSolver::assertConcretization(const Query &query,
                                              const Assignment &assign) const {
  for (const Array *symcreteArray :
       query.constraints.gatherSymcretizedArrays()) {
    if (!assign.bindings.count(symcreteArray)) {
      return false;
    }
  }
  return true;
}

bool ConcretizingSolver::getBrokenArrays(
    const Query &query, const Assignment &assign, ref<SolverResponse> &result,
    std::vector<const Array *> &brokenArrays) {
  Query concretizedQuery = constructConcretizedQuery(query, assign);
  ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(concretizedQuery.expr);
  if (CE && CE->isTrue()) {
    findObjects(query.expr, brokenArrays);
    result = new UnknownResponse();
    return true;
  }
  const ExprHashMap<ref<Expr>> &reverse =
      concretizedQuery.constraints.independentElements().concretizedExprs;
  ref<Expr> reverseExpr = query.expr;
  if (!solver->impl->check(concretizedQuery, result)) {
    return false;
  }
  reverseConcretization(result, reverse, reverseExpr);

  /* No unsat cores were found for the query, so we can try to find new
   * solution. */
  if (isa<InvalidResponse>(result)) {
    return true;
  }

  ValidityCore validityCore;
  bool success = result->tryGetValidityCore(validityCore);
  assert(success);

  constraints_ty allValidityCoreConstraints = validityCore.constraints;
  allValidityCoreConstraints.insert(validityCore.expr);
  findObjects(allValidityCoreConstraints.begin(),
              allValidityCoreConstraints.end(), brokenArrays);
  return true;
}

bool ConcretizingSolver::relaxSymcreteConstraints(const Query &query,
                                                  ref<SolverResponse> &result) {
  if (!query.containsSizeSymcretes()) {
    result = new UnknownResponse({});
    return true;
  }

  /* Get initial symcrete solution. We will try to relax them in order to
   * achieve `mayBeTrue` solution. */
  Assignment assignment = query.constraints.concretization();

  /* Create mapping from arrays to symcretes in order to determine which
   * symcretes break (i.e. can not have current value) with given array. */
  std::unordered_map<const Array *, std::vector<ref<Symcrete>>>
      symcretesDependentFromArrays;
  std::unordered_map<const Array *, std::unordered_set<Symcrete::SymcreteKind>>
      kindsOfSymcretesForArrays;

  for (const ref<Symcrete> &symcrete : query.constraints.symcretes()) {
    for (const Array *array : symcrete->dependentArrays()) {
      symcretesDependentFromArrays[array].push_back(symcrete);
      kindsOfSymcretesForArrays[array].insert(symcrete->getKind());
    }
  }

  std::set<const Array *> usedSymcretizedArrays;
  SymcreteOrderedSet brokenSymcretes;

  bool wereConcretizationsRemoved = true;
  while (wereConcretizationsRemoved) {
    wereConcretizationsRemoved = false;

    std::vector<const Array *> currentlyBrokenSymcretizedArrays;
    if (!getBrokenArrays(query, assignment, result,
                         currentlyBrokenSymcretizedArrays)) {
      return false;
    }

    if (isa<InvalidResponse>(result)) {
      break;
    }

    std::queue<const Array *> arrayQueue;
    std::queue<const Array *> addressQueue;

    bool addressArrayPresent = false;
    for (const Array *array : currentlyBrokenSymcretizedArrays) {
      if (symcretesDependentFromArrays.count(array) &&
          (kindsOfSymcretesForArrays.at(array).count(
               Symcrete::SymcreteKind::SK_ALLOC_SIZE) ||
           kindsOfSymcretesForArrays.at(array).count(
               Symcrete::SymcreteKind::SK_LI_SIZE)) &&
          usedSymcretizedArrays.insert(array).second) {
        arrayQueue.push(array);
      }
      if (symcretesDependentFromArrays.count(array) &&
          (kindsOfSymcretesForArrays.at(array).count(
               Symcrete::SymcreteKind::SK_ALLOC_ADDRESS) ||
           kindsOfSymcretesForArrays.at(array).count(
               Symcrete::SymcreteKind::SK_LI_ADDRESS)) &&
          !usedSymcretizedArrays.count(array)) {
        addressArrayPresent = true;
        addressQueue.push(array);
      }
    }

    if (arrayQueue.empty() && addressArrayPresent) {
      while (!addressQueue.empty()) {
        assignment.bindings.remove(addressQueue.front());
        addressQueue.pop();
      }

      if (!solver->impl->check(constructConcretizedQuery(query, assignment),
                               result)) {
        return false;
      } else {
        if (isa<InvalidResponse>(result)) {
          result = new UnknownResponse({});
        }
        return true;
      }
    }

    while (!arrayQueue.empty()) {
      const Array *brokenArray = arrayQueue.front();
      assignment.bindings.remove(brokenArray);
      wereConcretizationsRemoved = true;
      arrayQueue.pop();

      SymcreteOrderedSet currentlyBrokenSymcretes;

      for (const ref<Symcrete> &symcrete :
           symcretesDependentFromArrays.at(brokenArray)) {
        /* Symcrete already have deleted. */
        if (brokenSymcretes.count(symcrete)) {
          continue;
        }

        if (isa<SizeSymcrete>(symcrete)) {
          currentlyBrokenSymcretes.insert(symcrete);

          for (Symcrete &dependentSymcrete : symcrete->dependentSymcretes()) {
            currentlyBrokenSymcretes.insert(ref<Symcrete>(&dependentSymcrete));
          }
        }
      }

      for (const ref<Symcrete> &symcrete : currentlyBrokenSymcretes) {
        std::vector<ref<const IndependentConstraintSet>> factors;
        Query(query.constraints,
              AndExpr::create(query.expr,
                              Expr::createIsZero(symcrete->symcretized)))
            .getAllDependentConstraintsSets(factors);
        for (ref<const IndependentConstraintSet> ics : factors) {
          for (ref<Symcrete> symcrete : ics->symcretes) {
            currentlyBrokenSymcretes.insert(symcrete);
          }
        }
      }

      for (const ref<Symcrete> &symcrete : currentlyBrokenSymcretes) {
        brokenSymcretes.insert(symcrete);
        for (const Array *array : symcrete->dependentArrays()) {
          if (usedSymcretizedArrays.insert(array).second) {
            arrayQueue.push(array);
          }
        }
      }
    } // bfs end
  }

  if (isa<ValidResponse>(result)) {
    return true;
  }

  Query concretizedNegatedQuery =
      constructConcretizedQuery(query.negateExpr(), assignment);

  ConstraintSet queryConstraints = concretizedNegatedQuery.constraints;
  queryConstraints.addConstraint(concretizedNegatedQuery.expr, {});

  std::vector<ref<Expr>> sizeSymcretes;
  for (const ref<Symcrete> &symcrete : brokenSymcretes) {
    if (isa<SizeSymcrete>(symcrete)) {
      sizeSymcretes.push_back(symcrete->symcretized);
    }
  }

  assert(!sizeSymcretes.empty());
  ref<Expr> symbolicSizesSum = createNonOverflowingSumExpr(sizeSymcretes);
  symbolicSizesSum =
      Simplificator::simplifyExpr(query.constraints, symbolicSizesSum)
          .simplified;

  ref<SolverResponse> response;

  /* Compute assignment for symcretes. */
  std::vector<const Array *> objects(usedSymcretizedArrays.begin(),
                                     usedSymcretizedArrays.end());
  std::vector<SparseStorage<unsigned char>> brokenSymcretizedValues;

  if (!solver->impl->check(
          Query(queryConstraints,
                UgtExpr::create(
                    symbolicSizesSum,
                    ConstantExpr::create(SymbolicAllocationThreshold,
                                         symbolicSizesSum->getWidth()))),
          response)) {
    return false;
  }

  if (!response->tryGetInitialValuesFor(objects, brokenSymcretizedValues)) {
    ref<ConstantExpr> minimalValueOfSum;
    /* Receive model with a smallest sum as possible. */
    if (!solver->impl->computeMinimalUnsignedValue(
            Query(queryConstraints, symbolicSizesSum), minimalValueOfSum)) {
      return false;
    }

    bool hasSolution = false;
    if (!solver->impl->computeInitialValues(
            Query(queryConstraints,
                  EqExpr::create(symbolicSizesSum, minimalValueOfSum))
                .negateExpr(),
            objects, brokenSymcretizedValues, hasSolution)) {
      return false;
    }
    assert(hasSolution && "Symcretes values should have concretization after "
                          "computeInitialValues() query.");
  }

  for (unsigned idx = 0; idx < objects.size(); ++idx) {
    assignment.bindings.replace({objects[idx], brokenSymcretizedValues[idx]});
  }

  ExprHashMap<ref<Expr>> concretizations;

  for (ref<Symcrete> symcrete : query.constraints.symcretes()) {
    concretizations[symcrete->symcretized] =
        assignment.evaluate(symcrete->symcretized);
  }

  for (const ref<Symcrete> &symcrete : brokenSymcretes) {
    ref<SizeSymcrete> sizeSymcrete = dyn_cast<SizeSymcrete>(symcrete);

    if (!sizeSymcrete) {
      continue;
    }

    /* Receive address array linked with this size array to request address
     * concretization. */
    ref<Expr> condcretized = assignment.evaluate(symcrete->symcretized);

    uint64_t newSize = cast<ConstantExpr>(condcretized)->getZExtValue();

    void *address = addressGenerator->allocate(
        sizeSymcrete->addressSymcrete.symcretized, newSize);
    unsigned char *charAddressIterator =
        reinterpret_cast<unsigned char *>(&address);
    SparseStorage<unsigned char> storage(sizeof(address));
    storage.store(0, charAddressIterator,
                  charAddressIterator + sizeof(address));

    concretizations[sizeSymcrete->addressSymcrete.symcretized] =
        ConstantExpr::create(
            reinterpret_cast<uint64_t>(address),
            sizeSymcrete->addressSymcrete.symcretized->getWidth());
  }

  ref<Expr> concretizationCondition = query.expr;
  for (const auto &concretization : concretizations) {
    concretizationCondition =
        OrExpr::create(Expr::createIsZero(EqExpr::create(
                           concretization.first, concretization.second)),
                       concretizationCondition);
  }

  if (!solver->impl->check(query.withExpr(concretizationCondition), result)) {
    return false;
  }

  if (isa<ValidResponse>(result)) {
    result = new UnknownResponse();
  }

  return true;
}

bool ConcretizingSolver::computeValidity(const Query &query,
                                         PartialValidity &result) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValidity(query, result);
  }
  ref<SolverResponse> queryResult, negatedQueryResult;
  if (!computeValidity(query, queryResult, negatedQueryResult)) {
    return false;
  }

  if (isa<ValidResponse>(queryResult)) {
    result = PValidity::MustBeTrue;
  } else if (isa<ValidResponse>(negatedQueryResult)) {
    result = PValidity::MustBeFalse;
  } else {
    auto QInvalid = isa<InvalidResponse>(queryResult),
         NQInvalid = isa<InvalidResponse>(negatedQueryResult);
    if (QInvalid && NQInvalid) {
      result = PValidity::TrueOrFalse;
    } else if (QInvalid) {
      result = PValidity::MayBeFalse;
    } else if (NQInvalid) {
      result = PValidity::MayBeTrue;
    } else {
      result = PValidity::None;
    }
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
  auto assign = query.constraints.concretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  if (!check(query, queryResult) ||
      !check(query.negateExpr(), negatedQueryResult)) {
    return false;
  }
  return true;
}

bool ConcretizingSolver::check(const Query &query,
                               ref<SolverResponse> &result) {
  if (!query.containsSymcretes()) {
    return solver->impl->check(query, result);
  }
  auto assign = query.constraints.concretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  auto concretizedQuery = getConcretizedVersion(query);

  ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(concretizedQuery.expr);
  if (!CE || !CE->isTrue()) {
    if (!solver->impl->check(concretizedQuery, result)) {
      return false;
    }
  }
  if ((CE && CE->isTrue()) || isa<ValidResponse>(result)) {
    if (!relaxSymcreteConstraints(query, result)) {
      return false;
    }
  }

  return true;
}

char *ConcretizingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

bool ConcretizingSolver::computeTruth(const Query &query, bool &isValid) {
  if (!query.containsSymcretes()) {
    if (solver->impl->computeTruth(query, isValid)) {
      return true;
    }
    return false;
  }

  auto assign = query.constraints.concretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  if (ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(
          query.constraints.concretization().evaluate(query.expr))) {
    isValid = CE->isTrue();
  } else {
    auto concretizedQuery = getConcretizedVersion(query);
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
    if (isa<InvalidResponse>(result)) {
      isValid = false;
    }
  }

  return true;
}

bool ConcretizingSolver::computeValidityCore(const Query &query,
                                             ValidityCore &validityCore,
                                             bool &isValid) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValidityCore(query, validityCore, isValid);
  }
  Assignment assign = query.constraints.concretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  Query concretizedQuery = getConcretizedVersion(query);

  if (ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(
          query.constraints.concretization().evaluate(query.expr))) {
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
    if (isa<UnknownResponse>(result)) {
      return false;
    }
    /* Here we already have validity core from query above. */
    if (ref<InvalidResponse> resultInvalidResponse =
            dyn_cast<InvalidResponse>(result)) {
      assign = resultInvalidResponse->initialValuesFor(assign.keys());
      isValid = false;
    } else {
      bool success = result->tryGetValidityCore(validityCore);
      assert(success);
    }
  }

  if (!isValid) {
    validityCore = ValidityCore();
  }

  return true;
}

bool ConcretizingSolver::computeValue(const Query &query, ref<Expr> &result) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValue(query, result);
  }

  Assignment assign = query.constraints.concretization();

  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  if (ref<ConstantExpr> expr = dyn_cast<ConstantExpr>(
          query.constraints.concretization().evaluate(query.expr))) {
    result = expr;
    return true;
  }
  auto concretizedQuery = getConcretizedVersion(query);
  return solver->impl->computeValue(concretizedQuery, result);
}

bool ConcretizingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeInitialValues(query, objects, values,
                                              hasSolution);
  }

  Assignment assign = query.constraints.concretization();
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  auto concretizedQuery = getConcretizedVersion(query);
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
      result->tryGetInitialValuesFor(objects, values);
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

std::unique_ptr<Solver>
createConcretizingSolver(std::unique_ptr<Solver> s,
                         AddressGenerator *addressGenerator) {
  return std::make_unique<Solver>(
      std::make_unique<ConcretizingSolver>(std::move(s), addressGenerator));
}
} // namespace klee
