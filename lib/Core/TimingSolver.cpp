//===-- TimingSolver.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TimingSolver.h"

#include "klee/Config/Version.h"
#include "klee/ExecutionState.h"
#include "klee/Solver.h"
#include "klee/Statistics.h"
#include "klee/Internal/System/Time.h"
#include "SeedInfo.h"

#include "CoreStats.h"

#include "llvm/Support/TimeValue.h"

using namespace klee;
using namespace llvm;

/***/
ref<Expr> TimingSolver::ZESTEvaluate(const ExecutionState &state,
                                     ref<Expr> expr) {
  std::map<ExecutionState *, std::vector<SeedInfo> >::iterator its =
      seedMap->find(const_cast<ExecutionState *>(&state));

  // Paul need to understand better when this can happen
  if (its != seedMap->end()) {
    // Paul: need to understand better how the size of the seed vector varies
    assert(its->second.size() <= 1);
    std::vector<SeedInfo>::iterator siit = its->second.begin();

    if (siit != its->second.end()) {
      return siit->assignment.evaluate(expr);
    }
  }
  return expr;
}

bool TimingSolver::evaluate(const ExecutionState &state, ref<Expr> expr,
                            Solver::Validity &result, bool useSeeds) {
  // Fast path, to avoid timer and OS overhead.
  if (seedMap && useSeeds) {
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
      result = CE->isTrue() ? Solver::True : Solver::False;
      return true;
    }
    expr = ZESTEvaluate(state, expr);
  }
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? Solver::True : Solver::False;
    return true;
  }
  sys::TimeValue now = util::getWallTimeVal();

  if (simplifyExprs)
    expr = state.constraints.simplifyExpr(expr);

  bool success = solver->evaluate(Query(state.constraints, expr), result);

  sys::TimeValue delta = util::getWallTimeVal();
  delta -= now;
  stats::solverTime += delta.usec();
  state.queryCost += delta.usec()/1000000.;

  return success;
}

bool TimingSolver::mustBeTrue(const ExecutionState &state, ref<Expr> expr,
                              bool &result, bool useSeeds) {
  // Fast path, to avoid timer and OS overhead.
  if (seedMap && useSeeds) {
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
      result = CE->isTrue() ? true : false;
      return true;
    }
    expr = ZESTEvaluate(state, expr);
  }
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? true : false;
    return true;
  }

  sys::TimeValue now = util::getWallTimeVal();

  if (simplifyExprs)
    expr = state.constraints.simplifyExpr(expr);

  bool success = solver->mustBeTrue(Query(state.constraints, expr), result);

  sys::TimeValue delta = util::getWallTimeVal();
  delta -= now;
  stats::solverTime += delta.usec();
  state.queryCost += delta.usec()/1000000.;

  return success;
}

bool TimingSolver::mustBeFalse(const ExecutionState &state, ref<Expr> expr,
                               bool &result, bool useSeeds) {
  return mustBeTrue(state, Expr::createIsZero(expr), result, useSeeds);
}

bool TimingSolver::mayBeTrue(const ExecutionState &state, ref<Expr> expr,
                             bool &result, bool useSeeds) {
  bool res;
  if (!mustBeFalse(state, expr, res, useSeeds))
    return false;
  result = !res;
  return true;
}

bool TimingSolver::mayBeFalse(const ExecutionState &state, ref<Expr> expr,
                              bool &result, bool useSeeds) {
  bool res;
  if (!mustBeTrue(state, expr, res, useSeeds))
    return false;
  result = !res;
  return true;
}

bool TimingSolver::getValue(const ExecutionState &state, ref<Expr> expr,
                            ref<ConstantExpr> &result, bool useSeeds) {
  // Fast path, to avoid timer and OS overhead.
  if (seedMap && useSeeds) {
    expr = ZESTEvaluate(state, expr);
  }
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE;
    return true;
  }
  
  sys::TimeValue now = util::getWallTimeVal();

  if (simplifyExprs)
    expr = state.constraints.simplifyExpr(expr);

  bool success = solver->getValue(Query(state.constraints, expr), result);

  sys::TimeValue delta = util::getWallTimeVal();
  delta -= now;
  stats::solverTime += delta.usec();
  state.queryCost += delta.usec()/1000000.;

  return success;
}

bool TimingSolver::getInitialValues(
    const ExecutionState &state, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &result, bool useSeeds) {
  if (objects.empty())
    return true;

  sys::TimeValue now = util::getWallTimeVal();

  bool success = solver->getInitialValues(Query(state.constraints,
                                                ConstantExpr::alloc(0, Expr::Bool)), 
                                          objects, result);
  
  sys::TimeValue delta = util::getWallTimeVal();
  delta -= now;
  stats::solverTime += delta.usec();
  state.queryCost += delta.usec()/1000000.;
  
  return success;
}

std::pair<ref<Expr>, ref<Expr> >
TimingSolver::getRange(const ExecutionState &state, ref<Expr> expr,
                       int useSeeds) {
  if (seedMap && useSeeds) {
    expr = ZESTEvaluate(state, expr);
  }
  return solver->getRange(Query(state.constraints, expr));
}
