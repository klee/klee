//===-- TimingSolver.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TimingSolver.h"

#include "ExecutionState.h"

#include "klee/Config/Version.h"
#include "klee/Solver/Solver.h"
#include "klee/Statistics/Statistics.h"
#include "klee/Statistics/TimerStatIncrementer.h"

#include "CoreStats.h"

using namespace klee;
using namespace llvm;

/***/

bool TimingSolver::evaluate(const ConstraintSet &constraints, ref<Expr> expr,
                            Solver::Validity &result,
                            SolverQueryMetaData &metaData,
                            bool produceValidityCore) {
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? Solver::True : Solver::False;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = ConstraintManager::simplifyExpr(constraints, expr);

  ref<SolverResponse> queryResult;
  ref<SolverResponse> negatedQueryResult;

  bool success = produceValidityCore
                     ? solver->evaluate(Query(constraints, expr), queryResult,
                                        negatedQueryResult)
                     : solver->evaluate(Query(constraints, expr), result);

  if (success && produceValidityCore) {
    if (isa<ValidResponse>(queryResult) &&
        isa<InvalidResponse>(negatedQueryResult)) {
      result = Solver::True;
    } else if (isa<InvalidResponse>(queryResult) &&
               isa<ValidResponse>(negatedQueryResult)) {
      result = Solver::False;
    } else if (isa<InvalidResponse>(queryResult) &&
               isa<InvalidResponse>(negatedQueryResult)) {
      result = Solver::Unknown;
    } else {
      assert(0 && "unreachable");
    }
  }

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::tryGetUnique(const ConstraintSet &constraints, ref<Expr> e,
                                ref<Expr> &result,
                                SolverQueryMetaData &metaData) {
  result = e;
  if (!isa<ConstantExpr>(result)) {
    ref<ConstantExpr> value;
    bool isTrue = false;

    e = optimizer.optimizeExpr(e, true);
    TimerStatIncrementer timer(stats::solverTime);

    if (!solver->getValue(Query(constraints, e), value)) {
      return false;
    }
    ref<Expr> cond = EqExpr::create(e, value);
    cond = optimizer.optimizeExpr(cond, false);
    if (!solver->mustBeTrue(Query(constraints, cond), isTrue)) {
      return false;
    }
    if (isTrue) {
      result = value;
    }

    metaData.queryCost += timer.delta();
  }

  return true;
}

bool TimingSolver::mustBeTrue(const ConstraintSet &constraints, ref<Expr> expr,
                              bool &result, SolverQueryMetaData &metaData,
                              bool produceValidityCore) {
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? true : false;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = ConstraintManager::simplifyExpr(constraints, expr);

  ValidityCore validityCore;

  bool success = produceValidityCore
                     ? solver->getValidityCore(Query(constraints, expr),
                                               validityCore, result)
                     : solver->mustBeTrue(Query(constraints, expr), result);

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::mustBeFalse(const ConstraintSet &constraints, ref<Expr> expr,
                               bool &result, SolverQueryMetaData &metaData,
                               bool produceValidityCore) {
  return mustBeTrue(constraints, Expr::createIsZero(expr), result, metaData,
                    produceValidityCore);
}

bool TimingSolver::mayBeTrue(const ConstraintSet &constraints, ref<Expr> expr,
                             bool &result, SolverQueryMetaData &metaData,
                             bool produceValidityCore) {
  bool res;
  if (!mustBeFalse(constraints, expr, res, metaData, produceValidityCore))
    return false;
  result = !res;
  return true;
}

bool TimingSolver::mayBeFalse(const ConstraintSet &constraints, ref<Expr> expr,
                              bool &result, SolverQueryMetaData &metaData,
                              bool produceValidityCore) {
  bool res;
  if (!mustBeTrue(constraints, expr, res, metaData, produceValidityCore))
    return false;
  result = !res;
  return true;
}

bool TimingSolver::getValue(const ConstraintSet &constraints, ref<Expr> expr,
                            ref<ConstantExpr> &result,
                            SolverQueryMetaData &metaData) {
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = ConstraintManager::simplifyExpr(constraints, expr);

  bool success = solver->getValue(Query(constraints, expr), result);

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::getMinimalUnsignedValue(const ConstraintSet &constraints,
                                           ref<Expr> expr,
                                           ref<ConstantExpr> &result,
                                           SolverQueryMetaData &metaData) {
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = ConstraintManager::simplifyExpr(constraints, expr);

  bool success =
      solver->getMinimalUnsignedValue(Query(constraints, expr), result);

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::getInitialValues(
    const ConstraintSet &constraints, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &result,
    SolverQueryMetaData &metaData, bool produceValidityCore) {
  if (objects.empty())
    return true;

  TimerStatIncrementer timer(stats::solverTime);

  ref<SolverResponse> queryResult;

  bool success =
      produceValidityCore
          ? solver->check(
                Query(constraints, ConstantExpr::alloc(0, Expr::Bool)),
                queryResult)
          : solver->getInitialValues(
                Query(constraints, ConstantExpr::alloc(0, Expr::Bool)), objects,
                result);

  if (success && produceValidityCore && isa<InvalidResponse>(queryResult)) {
    success = queryResult->tryGetInitialValuesFor(objects, result);
  }

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::evaluate(const ConstraintSet &constraints, ref<Expr> expr,
                            ref<SolverResponse> &queryResult,
                            ref<SolverResponse> &negatedQueryResult,
                            SolverQueryMetaData &metaData) {
  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = ConstraintManager::simplifyExpr(constraints, expr);

  bool success = solver->evaluate(Query(constraints, expr), queryResult,
                                  negatedQueryResult);

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::getValidityCore(const ConstraintSet &constraints,
                                   ref<Expr> expr, ValidityCore &validityCore,
                                   bool &result,
                                   SolverQueryMetaData &metaData) {
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? true : false;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = ConstraintManager::simplifyExpr(constraints, expr);

  bool success =
      solver->getValidityCore(Query(constraints, expr), validityCore, result);

  metaData.queryCost += timer.delta();

  return success;
}

std::pair<ref<Expr>, ref<Expr>>
TimingSolver::getRange(const ConstraintSet &constraints, ref<Expr> expr,
                       SolverQueryMetaData &metaData, time::Span timeout) {
  TimerStatIncrementer timer(stats::solverTime);
  auto query = Query(constraints, expr);
  auto result = solver->getRange(query, timeout);
  metaData.queryCost += timer.delta();
  return result;
}
