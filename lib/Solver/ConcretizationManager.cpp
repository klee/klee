#include "klee/Solver/ConcretizationManager.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Solver/Solver.h"
#include <set>

using namespace klee;

Assignment ConcretizationManager::get(const ConstraintSet &set,
                                      ref<Expr> query) {
  Assignment assign(true);

  CacheEntry ce(set, query);
  concretizations_map::iterator it = concretizations.find(ce);
  if (it != concretizations.end()) {
    const Assignment &a = it->second;
    for (auto i : a.bindings) {
      assign.bindings.insert(i);
    }
  } else if (!set.empty()) {
    assert(0);
  }

  return assign;
}

bool ConcretizationManager::contains(const ConstraintSet &set,
                                     ref<Expr> query) {
  CacheEntry ce(set, query);
  concretizations_map::iterator it = concretizations.find(ce);
  return it != concretizations.end();
}

void ConcretizationManager::add(const Query &query, const Assignment &assign) {
  CacheEntry ce(query.constraints, query.expr);
  concretizations.insert(std::make_pair(ce, assign));
}
