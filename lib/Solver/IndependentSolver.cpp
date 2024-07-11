//===-- IndependentSolver.cpp ---------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/SymbolicSource.h"
#include "klee/Solver/SolverUtil.h"

#define DEBUG_TYPE "independent-solver"
#include "klee/Solver/Solver.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentConstraintSetUnion.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Solver/SolverImpl.h"

#include <memory>
#include <utility>
#include <vector>

using namespace klee;
using namespace llvm;

class IndependentSolver : public SolverImpl {
private:
  std::unique_ptr<Solver> solver;

public:
  IndependentSolver(std::unique_ptr<Solver> solver)
      : solver(std::move(solver)) {}

  bool computeTruth(const Query &, bool &isValid);
  bool computeValidity(const Query &, PartialValidity &result);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(
      const Query &query, const std::vector<const Array *> &objects,
      std::vector<SparseStorageImpl<unsigned char>> &values, bool &hasSolution);
  bool check(const Query &query, ref<SolverResponse> &result);
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query &);
  void setCoreSolverTimeout(time::Span timeout);
  void notifyStateTermination(std::uint32_t id);
};

bool IndependentSolver::computeValidity(const Query &query,
                                        PartialValidity &result) {
  std::vector<ref<const IndependentConstraintSet>> factors;
  query.getAllDependentConstraintsSets(factors);
  ConstraintSet tmp(factors,
                    query.constraints.independentElements().concretizedExprs);
  return solver->impl->computeValidity(query.withConstraints(tmp), result);
}

bool IndependentSolver::computeTruth(const Query &query, bool &isValid) {
  std::vector<ref<const IndependentConstraintSet>> factors;
  query.getAllDependentConstraintsSets(factors);
  ConstraintSet tmp(factors,
                    query.constraints.independentElements().concretizedExprs);
  return solver->impl->computeTruth(query.withConstraints(tmp), isValid);
}

bool IndependentSolver::computeValue(const Query &query, ref<Expr> &result) {
  std::vector<ref<const IndependentConstraintSet>> factors;
  query.getAllDependentConstraintsSets(factors);
  ConstraintSet tmp(factors,
                    query.constraints.independentElements().concretizedExprs);
  return solver->impl->computeValue(query.withConstraints(tmp), result);
}

// Helper function used only for assertions to make sure point created
// during computeInitialValues is in fact correct. The ``retMap`` is used
// in the case ``objects`` doesn't contain all the assignments needed.
bool assertCreatedPointEvaluatesToTrue(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorageImpl<unsigned char>> &values,
    Assignment::bindings_ty &retMap) {
  // _allowFreeValues is set to true so that if there are missing bytes in the
  // assignment we will end up with a non ConstantExpr after evaluating the
  // assignment and fail
  Assignment assign = Assignment(objects, values);

  // Add any additional bindings.
  // The semantics of std::map should be to not insert a (key, value)
  // pair if it already exists so we should continue to use the assignment
  // from ``objects`` and ``values``.
  if (retMap.size() > 0) {
    for (auto it : retMap) {
      assign.bindings.insert(it);
    }
  }
  for (auto const &constraint : query.constraints.cs()) {
    ref<Expr> ret = assign.evaluate(constraint);
    if (!isa<ConstantExpr>(ret)) {
      ret = ret->rebuild();
    }
    assert(isa<ConstantExpr>(ret) &&
           "assignment evaluation did not result in constant");
    ref<ConstantExpr> evaluatedConstraint = dyn_cast<ConstantExpr>(ret);
    if (evaluatedConstraint->isFalse()) {
      return false;
    }
  }
  ref<Expr> neg = Expr::createIsZero(query.expr);
  ref<Expr> q = assign.evaluate(neg);
  if (!isa<ConstantExpr>(q)) {
    q = q->rebuild();
  }
  assert(isa<ConstantExpr>(q) &&
         "assignment evaluation did not result in constant");
  return cast<ConstantExpr>(q)->isTrue();
}

bool assertCreatedPointEvaluatesToTrue(const Query &query,
                                       Assignment::bindings_ty &bindings,
                                       Assignment::bindings_ty &retMap) {
  std::vector<const Array *> objects;
  std::vector<SparseStorageImpl<unsigned char>> values;
  objects.reserve(bindings.size());
  values.reserve(bindings.size());
  for (auto &ovp : bindings) {
    objects.push_back(ovp.first);
    values.push_back(ovp.second);
  }
  return assertCreatedPointEvaluatesToTrue(query, objects, values, retMap);
}

bool IndependentSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorageImpl<unsigned char>> &values, bool &hasSolution) {
  // We assume the query has a solution except proven differently
  // This is important in case we don't have any constraints but
  // we need initial values for requested array objects.
  hasSolution = true;
  Assignment retMap;
  std::vector<ref<const IndependentConstraintSet>> dependentFactors;
  query.getAllDependentConstraintsSets(dependentFactors);
  std::vector<ref<const IndependentConstraintSet>> independentFactors;
  query.getAllIndependentConstraintsSets(independentFactors);

  if (independentFactors.size() == 0 && dependentFactors.size() == 1) {
    return solver->impl->computeInitialValues(query, objects, values,
                                              hasSolution);
  }

  ConstraintSet dependentConstriants(
      dependentFactors,
      query.constraints.independentElements().concretizedExprs);

  std::vector<const Array *> dependentFactorsObjects;
  calculateArraysInFactors(dependentFactors, query.expr,
                           dependentFactorsObjects);

  if (dependentFactorsObjects.size() != 0) {
    std::vector<SparseStorageImpl<unsigned char>> dependentFactorsValues;

    if (!solver->impl->computeInitialValues(
            query.withConstraints(dependentConstriants),
            dependentFactorsObjects, dependentFactorsValues, hasSolution)) {
      values.clear();
      return false;
    } else if (!hasSolution) {
      values.clear();
      return true;
    } else {
      retMap = Assignment(dependentFactorsObjects, dependentFactorsValues);
    }
  }

  // Used to rearrange all of the answers into the correct order
  for (ref<const IndependentConstraintSet> it : independentFactors) {
    std::vector<const Array *> arraysInFactor;
    it->calculateArrayReferences(arraysInFactor);
    // Going to use this as the "fresh" expression for the Query() invocation
    // below
    ConstraintSet tmp(it);
    std::vector<SparseStorageImpl<unsigned char>> tempValues;
    if (arraysInFactor.size() == 0) {
      continue;
    } else if (it->exprs.size() == 0) {
      ref<SolverResponse> tempResult = new InvalidResponse();
      [[maybe_unused]] bool success =
          tempResult->tryGetInitialValuesFor(arraysInFactor, tempValues);
      assert(success && "Can not get initial values (Independent solver)!");
    } else {
      if (!solver->impl->computeInitialValues(
              Query(tmp, Expr::createFalse(), query.id), arraysInFactor,
              tempValues, hasSolution)) {
        values.clear();
        return false;
      }
    }

    if (!hasSolution) {
      values.clear();

      return true;
    } else {
      assert(tempValues.size() == arraysInFactor.size() &&
             "Should be equal number arrays and answers");
      // We already have an array with some partially correct answers,
      // so we need to place the answers to the new query into the right
      // spot while avoiding the undetermined values also in the array
      it->addValuesToAssignment(arraysInFactor, tempValues, retMap);
    }
  }

  for (std::vector<const Array *>::const_iterator it = objects.begin();
       it != objects.end(); it++) {
    const Array *arr = *it;
    if (!retMap.bindings.count(arr)) {
      // this means we have an array that is somehow related to the
      // constraint, but whose values aren't actually required to
      // satisfy the query.
      ref<ConstantExpr> arrayConstantSize =
          dyn_cast<ConstantExpr>(retMap.evaluate(arr->size));
      assert(arrayConstantSize &&
             "Array of symbolic size had not receive value for size!");
      SparseStorageImpl<unsigned char> ret(0);
      values.push_back(ret);
    } else {
      values.push_back(retMap.bindings.at(arr));
    }
  }

  assert(assertCreatedPointEvaluatesToTrue(query, objects, values,
                                           retMap.bindings) &&
         "should satisfy the equation");

  return true;
}

bool IndependentSolver::check(const Query &query, ref<SolverResponse> &result) {
  // We assume the query has a solution except proven differently
  // This is important in case we don't have any constraints but
  // we need initial values for requested array objects.
  // result = new ValidResponse(ValidityCore());
  Assignment retMap;
  std::vector<ref<const IndependentConstraintSet>> dependentFactors;
  query.getAllDependentConstraintsSets(dependentFactors);
  std::vector<ref<const IndependentConstraintSet>> independentFactors;
  query.getAllIndependentConstraintsSets(independentFactors);

  if (independentFactors.size() == 0 && dependentFactors.size() == 1) {
    return solver->impl->check(query, result);
  }

  ConstraintSet dependentConstriants(
      dependentFactors,
      query.constraints.independentElements().concretizedExprs);

  std::vector<const Array *> dependentFactorsObjects;
  std::vector<SparseStorageImpl<unsigned char>> dependentFactorsValues;
  ref<SolverResponse> dependentFactorsResult;

  calculateArraysInFactors(dependentFactors, query.expr,
                           dependentFactorsObjects);

  if (dependentFactorsObjects.size() != 0) {
    if (!solver->impl->check(query.withConstraints(dependentConstriants),
                             dependentFactorsResult)) {
      return false;
    } else if (isa<ValidResponse>(dependentFactorsResult)) {
      result = dependentFactorsResult;
      return true;
    } else {
      [[maybe_unused]] bool success =
          dependentFactorsResult->tryGetInitialValuesFor(
              dependentFactorsObjects, dependentFactorsValues);
      assert(success && "Can not get initial values (Independent solver)!");
      retMap = Assignment(dependentFactorsObjects, dependentFactorsValues);
    }
  }

  // Used to rearrange all of the answers into the correct order
  for (ref<const IndependentConstraintSet> it : independentFactors) {
    std::vector<const Array *> arraysInFactor;
    it->calculateArrayReferences(arraysInFactor);
    // Going to use this as the "fresh" expression for the Query() invocation
    // below
    ref<SolverResponse> tempResult;
    std::vector<SparseStorageImpl<unsigned char>> tempValues;
    if (arraysInFactor.size() == 0) {
      continue;
    } else if (it->exprs.size() == 0) {
      tempResult = new InvalidResponse();
    } else {
      if (!solver->impl->check(
              Query(ConstraintSet(it), Expr::createFalse(), query.id),
              tempResult)) {
        return false;
      }
    }

    if (isa<ValidResponse>(tempResult)) {
      result = tempResult;
      return true;
    } else {
      [[maybe_unused]] bool success =
          tempResult->tryGetInitialValuesFor(arraysInFactor, tempValues);
      assert(success && "Can not get initial values (Independent solver)!");
      assert(tempValues.size() == arraysInFactor.size() &&
             "Should be equal number arrays and answers");
      // We already have an array with some partially correct answers,
      // so we need to place the answers to the new query into the right
      // spot while avoiding the undetermined values also in the array
      it->addValuesToAssignment(arraysInFactor, tempValues, retMap);
    }
  }
  result = new InvalidResponse(retMap.bindings);
  Assignment::bindings_ty bindings;
  [[maybe_unused]] bool success = result->tryGetInitialValues(bindings);
  assert(success);
  assert(assertCreatedPointEvaluatesToTrue(query, bindings, retMap.bindings) &&
         "should satisfy the equation");

  return true;
}

bool IndependentSolver::computeValidityCore(const Query &query,
                                            ValidityCore &validityCore,
                                            bool &isValid) {
  std::vector<ref<const IndependentConstraintSet>> factors;
  query.getAllDependentConstraintsSets(factors);
  ConstraintSet tmp(factors,
                    query.constraints.independentElements().concretizedExprs);
  return solver->impl->computeValidityCore(query.withConstraints(tmp),
                                           validityCore, isValid);
}

SolverImpl::SolverRunStatus IndependentSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

char *IndependentSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void IndependentSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

void IndependentSolver::notifyStateTermination(std::uint32_t id) {
  solver->impl->notifyStateTermination(id);
}

std::unique_ptr<Solver>
klee::createIndependentSolver(std::unique_ptr<Solver> s) {
  return std::make_unique<Solver>(
      std::make_unique<IndependentSolver>(std::move(s)));
}
