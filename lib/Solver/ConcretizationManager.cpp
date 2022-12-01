#include "klee/Solver/ConcretizationManager.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Solver/Solver.h"
#include <set>

using namespace klee;

Assignment ConcretizationManager::get(const ConstraintSet &set) {
  Assignment assign(true);
  if (auto a = concretizations.lookup(set.asSet())) {
    for (auto i : a->bindings) {
      assign.bindings.insert(i);
    }
  }

  return assign;
}

void ConcretizationManager::add(const Query &query, const Assignment &assign) {
  Assignment newConcretization(true);

  if (auto a = concretizations.lookup(query.constraints.asSet())) {
    for (auto i : a->bindings) {
      newConcretization.bindings.insert(i);
    }
  }

  ConstraintSet newConstraints;
  ConstraintManager cm(newConstraints);

  for (auto i : query.constraints) {
    cm.addConstraint(i);
  }
  cm.addConstraint(query.expr);

  for (auto i : assign.bindings) {
    newConcretization.bindings[i.first] = i.second;
  }

  concretizations.insert(newConstraints.asSet(), newConcretization);
}

Query ConcretizationManager::getConcretizedQuery(const Query &query) {
  if (auto assign = concretizations.lookup(query.constraints.asSet())) {
    ConstraintSet constraints;
    for (auto e : query.constraints) {
      constraints.push_back(assign->evaluate(e));
    }
    ref<Expr> expr = assign->evaluate(query.expr);
    ConstraintSet equalities = assign->createConstraintsFromAssignment();
    for (auto e : equalities) {
      constraints.push_back(e);
    }
    return Query(constraints, expr);
  }
  return query;
}
