#include "klee/Solver/ConcretizationManager.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Solver/Solver.h"
#include <assert.h>
#include <set>

using namespace klee;

std::pair<Assignment, bool> ConcretizationManager::get(const ConstraintSet &set,
                                                       ref<Expr> query) {
  if (simplifyExprs) {
    query = Simplificator::simplifyExpr(set, query).simplified;
  }
  CacheEntry ce(set.cs(), set.symcretes(), query);
  concretizations_map::iterator it = concretizations.find(ce);
  if (it != concretizations.end()) {
    return {it->second, true};
  } else {
    return {Assignment(true), false};
  }
}

bool ConcretizationManager::contains(const ConstraintSet &set,
                                     ref<Expr> query) {
  if (simplifyExprs) {
    query = Simplificator::simplifyExpr(set, query).simplified;
  }
  CacheEntry ce(set.cs(), set.symcretes(), query);
  concretizations_map::iterator it = concretizations.find(ce);
  return it != concretizations.end();
}

void ConcretizationManager::add(const Query &query, const Assignment &assign) {
  ref<Expr> expr = query.expr;
  if (simplifyExprs) {
    expr = Simplificator::simplifyExpr(query.constraints, expr).simplified;
  }
  CacheEntry ce(query.constraints.cs(), query.constraints.symcretes(), expr);
  concretizations.insert(std::make_pair(ce, assign));
  assert(concretizations.find(ce) != concretizations.end());
  assert(contains(query.constraints, expr));
}
