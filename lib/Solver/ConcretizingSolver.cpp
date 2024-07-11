//===-- ConcretizingSolver.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver/Solver.h"

#include "klee/ADT/MapOfSets.h"
#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentConstraintSetUnion.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Expr/Symcrete.h"

#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Solver/SolverUtil.h"

#include <queue>
#include <vector>

namespace klee {

typedef std::set<ref<Expr>> KeyType;

class ConcretizingSolver : public SolverImpl {
private:
  std::unique_ptr<Solver> solver;
  MapOfSets<ref<Expr>, Assignment> cache;

public:
  ConcretizingSolver(std::unique_ptr<Solver> _solver)
      : solver(std::move(_solver)) {}

  ~ConcretizingSolver();

  bool computeTruth(const Query &, bool &isValid);
  bool computeValidity(const Query &, PartialValidity &result);
  bool computeValidity(const Query &query, ref<SolverResponse> &queryResult,
                       ref<SolverResponse> &negatedQueryResult);

  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid);
  bool check(const Query &query, ref<SolverResponse> &result);

  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(
      const Query &query, const std::vector<const Array *> &objects,
      std::vector<SparseStorageImpl<unsigned char>> &values, bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);
  void notifyStateTermination(std::uint32_t id);

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
  bool updateConcretization(const Query &query, Assignment &assign);
};

KeyType makeKey(const Query &query) {
  KeyType key =
      KeyType(query.constraints.cs().begin(), query.constraints.cs().end());
  for (ref<Symcrete> s : query.constraints.symcretes()) {
    key.insert(s->symcretized);
  }
  ref<Expr> neg = Expr::createIsZero(query.expr);
  key.insert(neg);
  return key;
}

Query ConcretizingSolver::constructConcretizedQuery(const Query &query,
                                                    const Assignment &assign) {
  ConstraintSet cs = query.constraints;
  ref<Expr> concretizedExpr = assign.evaluate(query.expr);
  return Query(cs.getConcretizedVersion(assign), concretizedExpr, query.id);
}

Query ConcretizingSolver::getConcretizedVersion(const Query &query) {
  ConstraintSet cs = query.constraints;
  ref<Expr> concretizedExpr = cs.concretization().evaluate(query.expr);
  return Query(cs.getConcretizedVersion(), concretizedExpr, query.id);
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
    result = new ValidResponse(ValidityCore().withExpr(query.expr));
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
  [[maybe_unused]] bool success = result->tryGetValidityCore(validityCore);
  assert(success);

  constraints_ty allValidityCoreConstraints = validityCore.constraints;
  allValidityCoreConstraints.insert(validityCore.expr);
  findObjects(allValidityCoreConstraints.begin(),
              allValidityCoreConstraints.end(), brokenArrays);
  return true;
}

bool ConcretizingSolver::relaxSymcreteConstraints(const Query &query,
                                                  ref<SolverResponse> &result) {

  /* Get initial symcrete solution. We will try to relax them in order to
   * achieve `mayBeTrue` solution. */
  Assignment assignment = query.constraints.concretization();

  /* Create mapping from arrays to symcretes in order to determine which
   * symcretes break (i.e. can not have current value) with given array. */
  std::unordered_map<const Array *, std::vector<ref<Symcrete>>>
      symcretesDependentFromArrays;

  for (const ref<Symcrete> &symcrete : query.constraints.symcretes()) {
    for (const Array *array : symcrete->dependentArrays()) {
      symcretesDependentFromArrays[array].push_back(symcrete);
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

    [[maybe_unused]] bool addressArrayPresent = false;
    for (const Array *array : currentlyBrokenSymcretizedArrays) {
      if (symcretesDependentFromArrays.count(array) &&
          usedSymcretizedArrays.insert(array).second) {
        arrayQueue.push(array);
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

        currentlyBrokenSymcretes.insert(symcrete);
      }

      for (const ref<Symcrete> &symcrete : currentlyBrokenSymcretes) {
        std::vector<ref<const IndependentConstraintSet>> factors;
        query
            .withExpr(AndExpr::create(
                query.expr, Expr::createIsZero(symcrete->symcretized)))
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

  assignment = cast<InvalidResponse>(result)->initialValuesFor(
      query.constraints.gatherSymcretizedArrays());

  ExprHashMap<ref<Expr>> concretizations;

  for (ref<Symcrete> symcrete : query.constraints.symcretes()) {
    concretizations[symcrete->symcretized] =
        cast<ConstantExpr>(assignment.evaluate(symcrete->symcretized));
  }

  ref<Expr> concretizationCondition = query.expr;
  for (const auto &concretization : concretizations) {
    concretizationCondition =
        OrExpr::create(Expr::createIsZero(EqExpr::create(
                           concretization.first, concretization.second)),
                       concretizationCondition);
  }

  return solver->impl->check(query.withExpr(concretizationCondition), result);
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
  return result != PValidity::None;
}

bool ConcretizingSolver::computeValidity(
    const Query &query, ref<SolverResponse> &queryResult,
    ref<SolverResponse> &negatedQueryResult) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValidity(query, queryResult,
                                         negatedQueryResult);
  }

  if (!check(query, queryResult) ||
      !check(query.negateExpr(), negatedQueryResult)) {
    return false;
  }
  return true;
}

bool ConcretizingSolver::updateConcretization(const Query &query,
                                              Assignment &assign) {
  Query qwf = query.withFalse();
  ref<SolverResponse> result;
  KeyType key = makeKey(qwf);
  const Assignment *lookup = cache.lookup(key);
  if (lookup) {
    assign = *lookup;
  } else {
    if (!solver->impl->check(qwf, result)) {
      return false;
    }
    assert(isa<InvalidResponse>(result));

    assign = cast<InvalidResponse>(result)->initialValuesFor(
        qwf.constraints.gatherSymcretizedArrays());
    cache.insert(key, assign);
  }
  Assignment delta = qwf.constraints.concretization().diffWith(assign);
  qwf.constraints.rewriteConcretization(delta);
  return true;
}

bool ConcretizingSolver::check(const Query &query,
                               ref<SolverResponse> &result) {
  if (!query.containsSymcretes()) {
    return solver->impl->check(query, result);
  }

  Assignment assign;
  if (!updateConcretization(query, assign)) {
    return false;
  }

  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  auto concretizedQuery = constructConcretizedQuery(query, assign);

  ref<ConstantExpr> CE = dyn_cast<ConstantExpr>(concretizedQuery.expr);
  if (!CE || !CE->isTrue()) {
    if (!solver->impl->check(concretizedQuery, result)) {
      return false;
    }
  }
  if ((CE && CE->isTrue()) || (isa<ValidResponse>(result))) {
    if (!relaxSymcreteConstraints(query, result)) {
      return false;
    }
  }

  if (isa<ValidResponse>(result)) {
    for (auto i : cast<ValidResponse>(result)->validityCore().constraints) {
      if (!query.constraints.cs().count(i)) {
        return false;
      }
    }
  }

  if (isa<InvalidResponse>(result)) {
    KeyType key = makeKey(query);
    assign = cast<InvalidResponse>(result)->initialValuesFor(
        query.constraints.gatherSymcretizedArrays());
    cache.insert(key, assign);
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

  Assignment assign;
  if (!updateConcretization(query, assign)) {
    return false;
  }
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  if (ref<ConstantExpr> CE =
          dyn_cast<ConstantExpr>(assign.evaluate(query.expr))) {
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
    if (isa<InvalidResponse>(result)) {
      isValid = false;
    }
  }

  if (isa<InvalidResponse>(result)) {
    KeyType key = makeKey(query);
    assign = cast<InvalidResponse>(result)->initialValuesFor(
        query.constraints.gatherSymcretizedArrays());
    cache.insert(key, assign);
  }

  return true;
}

bool ConcretizingSolver::computeValidityCore(const Query &query,
                                             ValidityCore &validityCore,
                                             bool &isValid) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeValidityCore(query, validityCore, isValid);
  }
  Assignment assign;
  if (!updateConcretization(query, assign)) {
    return false;
  }
  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  Query concretizedQuery = constructConcretizedQuery(query, assign);

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
    /* Here we already have validity core from query above. */
    if (ref<InvalidResponse> resultInvalidResponse =
            dyn_cast<InvalidResponse>(result)) {
      KeyType key = makeKey(query);
      assign = resultInvalidResponse->initialValuesFor(
          query.constraints.gatherSymcretizedArrays());
      cache.insert(key, assign);
      isValid = false;
    } else {
      [[maybe_unused]] bool success = result->tryGetValidityCore(validityCore);
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

  Assignment assign;
  if (!updateConcretization(query, assign)) {
    return false;
  }

  assert(assertConcretization(query, assign) &&
         "Assignment does not contain concretization for all symcrete arrays!");

  if (ref<ConstantExpr> expr = dyn_cast<ConstantExpr>(
          query.constraints.concretization().evaluate(query.expr))) {
    result = expr;
    return true;
  }
  auto concretizedQuery = constructConcretizedQuery(query, assign);
  return solver->impl->computeValue(concretizedQuery, result);
}

bool ConcretizingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorageImpl<unsigned char>> &values, bool &hasSolution) {
  if (!query.containsSymcretes()) {
    return solver->impl->computeInitialValues(query, objects, values,
                                              hasSolution);
  }

  Assignment assign;
  if (!updateConcretization(query, assign)) {
    return false;
  }

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
      KeyType key = makeKey(query);
      assign = resultInvalidResponse->initialValuesFor(
          query.constraints.gatherSymcretizedArrays());
      cache.insert(key, assign);
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

void ConcretizingSolver::notifyStateTermination(std::uint32_t id) {
  solver->impl->notifyStateTermination(id);
}

ConcretizingSolver::~ConcretizingSolver() { cache.clear(); }

std::unique_ptr<Solver> createConcretizingSolver(std::unique_ptr<Solver> s) {
  return std::make_unique<Solver>(
      std::make_unique<ConcretizingSolver>(std::move(s)));
}
} // namespace klee
