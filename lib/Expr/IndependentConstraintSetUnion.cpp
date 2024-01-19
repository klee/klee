#include "klee/Expr/IndependentConstraintSetUnion.h"

#include "klee/ADT/DisjointSetUnion.h"
#include "klee/Expr/IndependentSet.h"

namespace klee {

IndependentConstraintSetUnion::IndependentConstraintSetUnion() {}

IndependentConstraintSetUnion::IndependentConstraintSetUnion(
    const constraints_ty &is, const SymcreteOrderedSet &os,
    const Assignment &c) {
  for (ref<Expr> e : is) {
    addExpr(e);
  }
  for (ref<Symcrete> s : os) {
    addSymcrete(s);
  }
  updateConcretization(c);
}

IndependentConstraintSetUnion::IndependentConstraintSetUnion(
    ref<const IndependentConstraintSet> ics) {
  auto exprs = ics->exprs;
  for (ref<Expr> e : exprs) {
    auto v = ref<ExprOrSymcrete::left>(new ExprOrSymcrete::left(e));
    rank.insert_or_assign(v, 0);
    internalStorage.insert(v);
  }

  for (ref<Symcrete> s : ics->symcretes) {
    auto v = ref<ExprOrSymcrete::right>(new ExprOrSymcrete::right(s));
    rank.insert_or_assign(v, 0);
    internalStorage.insert(v);
  }

  if (internalStorage.size() == 0) {
    return;
  }

  auto &first = *(internalStorage.begin());
  for (auto &e : internalStorage) {
    parent.insert_or_assign(e, first);
  }
  rank.insert_or_assign(first, 1);
  roots.insert(first);
  disjointSets.insert_or_assign(first, ics);
  concretization = ics->concretization;
}

void IndependentConstraintSetUnion::addIndependentConstraintSetUnion(
    const IndependentConstraintSetUnion &icsu) {
  add(icsu);
  concretization.addIndependentAssignment(icsu.concretization);
  updateQueue.addIndependentAssignment(icsu.updateQueue);
  removeQueue.addIndependentAssignment(icsu.removeQueue);
  constraintQueue.insert(constraintQueue.begin(), icsu.constraintQueue.begin(),
                         icsu.constraintQueue.end());
}

void IndependentConstraintSetUnion::updateConcretization(
    const Assignment &delta) {
  for (auto &it : delta.bindings) {
    updateQueue.bindings.replace({it.first, it.second});
    removeQueue.bindings.remove(it.first);
  }
}

void IndependentConstraintSetUnion::removeConcretization(
    const Assignment &remove) {
  for (auto &it : remove.bindings) {
    removeQueue.bindings.replace({it.first, it.second});
    updateQueue.bindings.remove(it.first);
  }
}

void IndependentConstraintSetUnion::calculateUpdateConcretizationQueue() {
  for (auto &e : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(e);
    Assignment part = updateQueue.part(ics->getSymcretes());
    ics = ics->updateConcretization(part, concretizedExprs);
    disjointSets.insert_or_assign(e, ics);
  }
  for (auto &it : updateQueue.bindings) {
    concretization.bindings.replace({it.first, it.second});
  }
}

void IndependentConstraintSetUnion::calculateRemoveConcretizationQueue() {
  for (auto &e : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(e);
    Assignment part = removeQueue.part(ics->getSymcretes());
    ics = ics->removeConcretization(part, concretizedExprs);
    disjointSets.insert_or_assign(e, ics);
  }
  for (auto &it : removeQueue.bindings) {
    concretization.bindings.remove(it.first);
  }
}

void IndependentConstraintSetUnion::reEvaluateConcretization(
    const Assignment &newConcretization) {
  Assignment delta;
  Assignment removed;
  for (const auto &it : concretization) {
    if (newConcretization.bindings.count(it.first) == 0) {
      removed.bindings.insert(it);
    } else if (newConcretization.bindings.at(it.first) != it.second) {
      delta.bindings.insert(*(newConcretization.bindings.find(it.first)));
    }
  }
  updateConcretization(delta);
  removeConcretization(removed);
}

void IndependentConstraintSetUnion::getAllIndependentConstraintSets(
    ref<Expr> e, std::vector<ref<const IndependentConstraintSet>> &result) {
  calculateQueue();
  ref<const IndependentConstraintSet> compare =
      new IndependentConstraintSet(new ExprOrSymcrete::left(e));
  for (auto &r : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(r);
    if (!IndependentConstraintSet::intersects(ics, compare)) {
      result.push_back(ics);
    }
  }
}

void IndependentConstraintSetUnion::getAllDependentConstraintSets(
    ref<Expr> e, std::vector<ref<const IndependentConstraintSet>> &result) {
  calculateQueue();
  ref<const IndependentConstraintSet> compare =
      new IndependentConstraintSet(new ExprOrSymcrete::left(e));
  for (auto &r : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(r);
    if (IndependentConstraintSet::intersects(ics, compare)) {
      result.push_back(ics);
    }
  }
}

void IndependentConstraintSetUnion::addExpr(ref<Expr> e) {
  constraintQueue.push_back(new ExprOrSymcrete::left(e));
}

void IndependentConstraintSetUnion::addSymcrete(ref<Symcrete> s) {
  constraintQueue.push_back(new ExprOrSymcrete::right(s));
}

IndependentConstraintSetUnion
IndependentConstraintSetUnion::getConcretizedVersion() {
  calculateQueue();
  IndependentConstraintSetUnion icsu;
  for (auto &i : roots) {
    ref<const IndependentConstraintSet> root = disjointSets.at(i);
    if (root->concretization.bindings.empty()) {
      for (ref<Expr> expr : root->exprs) {
        icsu.addExpr(expr);
      }
    } else {
      root->concretizedSets->calculateQueue();
      icsu.add(*root->concretizedSets.get());
    }
    icsu.concretization.addIndependentAssignment(root->concretization);
  }
  icsu.concretizedExprs = concretizedExprs;
  icsu.calculateQueue();
  return icsu;
}

IndependentConstraintSetUnion
IndependentConstraintSetUnion::getConcretizedVersion(
    const Assignment &newConcretization) {
  calculateQueue();
  IndependentConstraintSetUnion icsu = *this;
  icsu.reEvaluateConcretization(newConcretization);
  icsu.calculateQueue();
  return icsu.getConcretizedVersion();
}

void IndependentConstraintSetUnion::calculateQueue() {
  calculateUpdateConcretizationQueue();
  calculateRemoveConcretizationQueue();
  while (!constraintQueue.empty()) {
    auto constraint = constraintQueue[constraintQueue.size() - 1];
    if (auto expr = dyn_cast<ExprOrSymcrete::left>(constraint)) {
      if (auto ce = dyn_cast<ConstantExpr>(expr->value())) {
        assert(ce->isTrue() && "Attempt to add invalid constraint");
        constraintQueue.pop_back();
        continue;
      }
    }
    addValue(constraint);
    constraintQueue.pop_back();
  }
  calculateUpdateConcretizationQueue();
  calculateRemoveConcretizationQueue();
  // Calculations are done twice for constraints already in dsu and for newly
  // added constraints. Because IndependentSet update and remove concretization
  // functions work only with difference between new and old concretization, no
  // extra work is done
  removeQueue.bindings = Assignment::bindings_ty();
  updateQueue.bindings = Assignment::bindings_ty();
}
} // namespace klee
