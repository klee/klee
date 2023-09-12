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
#include "klee/Expr/Constraints.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverStats.h"
#include "klee/Solver/SolverUtil.h"
#include "klee/Statistics/Statistics.h"
#include "klee/Statistics/TimerStatIncrementer.h"

#include "CoreStats.h"

using namespace klee;
using namespace llvm;

/***/

bool TimingSolver::evaluate(const ConstraintSet &constraints, ref<Expr> expr,
                            PartialValidity &result,
                            SolverQueryMetaData &metaData,
                            bool produceValidityCore) {
  ++stats::queries;
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? PValidity::MustBeTrue : PValidity::MustBeFalse;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = Simplificator::simplifyExpr(constraints, expr).simplified;

  ref<SolverResponse> queryResult;
  ref<SolverResponse> negatedQueryResult;
  Query query(constraints, expr, metaData.id);

  bool success = produceValidityCore
                     ? solver->evaluate(query, queryResult, negatedQueryResult)
                     : solver->evaluate(query, result);

  if (success && produceValidityCore) {
    if (isa<ValidResponse>(queryResult) &&
        isa<InvalidResponse>(negatedQueryResult)) {
      result = PValidity::MustBeTrue;
    } else if (isa<InvalidResponse>(queryResult) &&
               isa<ValidResponse>(negatedQueryResult)) {
      result = PValidity::MustBeFalse;
    } else if (isa<InvalidResponse>(queryResult) &&
               isa<InvalidResponse>(negatedQueryResult)) {
      result = PValidity::TrueOrFalse;
    } else if (isa<InvalidResponse>(queryResult) &&
               isa<UnknownResponse>(negatedQueryResult)) {
      result = PValidity::MayBeFalse;
    } else if (isa<UnknownResponse>(queryResult) &&
               isa<InvalidResponse>(negatedQueryResult)) {
      result = PValidity::MayBeTrue;
    } else if (isa<UnknownResponse>(queryResult) &&
               isa<UnknownResponse>(negatedQueryResult)) {
      result = PValidity::None;
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
  ++stats::queries;
  result = e;
  if (!isa<ConstantExpr>(result)) {
    ref<ConstantExpr> value;
    bool isTrue = false;

    e = optimizer.optimizeExpr(e, true);
    TimerStatIncrementer timer(stats::solverTime);

    if (!solver->getValue(Query(constraints, e, metaData.id), value)) {
      return false;
    }
    ref<Expr> cond = EqExpr::create(e, value);
    cond = optimizer.optimizeExpr(cond, false);
    if (!solver->mustBeTrue(Query(constraints, cond, metaData.id), isTrue)) {
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
  ++stats::queries;
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? true : false;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = Simplificator::simplifyExpr(constraints, expr).simplified;

  ValidityCore validityCore;
  Query query(constraints, expr, metaData.id);

  bool success = produceValidityCore
                     ? solver->getValidityCore(query, validityCore, result)
                     : solver->mustBeTrue(query, result);

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
  ++stats::queries;
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = Simplificator::simplifyExpr(constraints, expr).simplified;

  bool success =
      solver->getValue(Query(constraints, expr, metaData.id), result);

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::getMinimalUnsignedValue(const ConstraintSet &constraints,
                                           ref<Expr> expr,
                                           ref<ConstantExpr> &result,
                                           SolverQueryMetaData &metaData) {
  ++stats::queries;
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs)
    expr = Simplificator::simplifyExpr(constraints, expr).simplified;

  bool success = solver->getMinimalUnsignedValue(
      Query(constraints, expr, metaData.id), result);

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::getInitialValues(
    const ConstraintSet &constraints, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &result,
    SolverQueryMetaData &metaData, bool produceValidityCore) {
  ++stats::queries;
  if (objects.empty())
    return true;

  TimerStatIncrementer timer(stats::solverTime);

  ref<SolverResponse> queryResult;
  Query query(constraints, Expr::createFalse(), metaData.id);

  bool success = produceValidityCore
                     ? solver->check(query, queryResult)
                     : solver->getInitialValues(query, objects, result);

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
  ++stats::queries;
  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs) {
    auto simplification = Simplificator::simplifyExpr(constraints, expr);
    auto simplified = simplification.simplified;
    auto dependency = simplification.dependency;
    if (auto CE = dyn_cast<ConstantExpr>(simplified)) {
      Query query(constraints, simplified, metaData.id);
      if (CE->isTrue()) {
        queryResult = new ValidResponse(ValidityCore(dependency, expr));
        return solver->check(query.negateExpr(), negatedQueryResult);
      } else {
        negatedQueryResult = new ValidResponse(
            ValidityCore(dependency, Expr::createIsZero(expr)));
        return solver->check(query, queryResult);
      }
    } else {
      expr = simplified;
    }
  }

  bool success = solver->evaluate(Query(constraints, expr, metaData.id),
                                  queryResult, negatedQueryResult);

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::getValidityCore(const ConstraintSet &constraints,
                                   ref<Expr> expr, ValidityCore &validityCore,
                                   bool &result,
                                   SolverQueryMetaData &metaData) {
  ++stats::queries;
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? true : false;
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs) {
    auto simplification = Simplificator::simplifyExpr(constraints, expr);
    auto simplifed = simplification.simplified;
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(simplifed)) {
      result = CE->isTrue();
      if (result) {
        validityCore = ValidityCore(simplification.dependency, expr);
      }
      return true;
    } else {
      expr = simplifed;
    }
  }

  bool success = solver->getValidityCore(Query(constraints, expr, metaData.id),
                                         validityCore, result);

  metaData.queryCost += timer.delta();

  return success;
}

bool TimingSolver::getResponse(const ConstraintSet &constraints, ref<Expr> expr,
                               ref<SolverResponse> &queryResult,
                               SolverQueryMetaData &metaData) {
  ++stats::queries;
  // Fast path, to avoid timer and OS overhead.
  if (expr->isTrue()) {
    queryResult = new ValidResponse(ValidityCore());
    return true;
  }

  TimerStatIncrementer timer(stats::solverTime);

  if (simplifyExprs) {
    auto simplification = Simplificator::simplifyExpr(constraints, expr);
    auto simplifed = simplification.simplified;
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(simplifed)) {
      if (CE->isTrue()) {
        queryResult =
            new ValidResponse(ValidityCore(simplification.dependency, expr));
        return true;
      }
    } else {
      expr = simplifed;
    }
  }

  bool success =
      solver->check(Query(constraints, expr, metaData.id), queryResult);

  metaData.queryCost += timer.delta();

  return success;
}

std::pair<ref<Expr>, ref<Expr>>
TimingSolver::getRange(const ConstraintSet &constraints, ref<Expr> expr,
                       SolverQueryMetaData &metaData, time::Span timeout) {
  ++stats::queries;
  TimerStatIncrementer timer(stats::solverTime);
  Query query(constraints, expr, metaData.id);
  auto result = solver->getRange(query, timeout);
  metaData.queryCost += timer.delta();
  return result;
}

void TimingSolver::notifyStateTermination(std::uint32_t id) {
  solver->notifyStateTermination(id);
}
