#include "klee/Expr/IndependentConstraintSetUnion.h"

namespace klee {

IndependentConstraintSetUnion::IndependentConstraintSetUnion() {}

IndependentConstraintSetUnion::IndependentConstraintSetUnion(
    const constraints_ty &is, const SymcreteOrderedSet &os,
    const Assignment &c) {
  for (ref<Expr> e : is) {
    addValue(e);
  }
  for (ref<Symcrete> s : os) {
    addSymcrete(s);
  }
  updateConcretization(c);
}

IndependentConstraintSetUnion::IndependentConstraintSetUnion(
    ref<const IndependentConstraintSet> ics) {
  for (ref<Expr> e : ics->exprs) {
    rank.replace({e, 0});
    internalStorage.insert(e);
    disjointSets.replace({e, nullptr});
  }

  for (ref<Symcrete> s : ics->symcretes) {
    ref<Expr> e = s->symcretized;
    rank.replace({e, 0});
    internalStorage.insert(e);
    disjointSets.replace({e, nullptr});
  }

  if (internalStorage.size() == 0) {
    return;
  }

  ref<Expr> first = *(internalStorage.begin());
  for (ref<Expr> e : internalStorage) {
    parent.replace({e, first});
  }
  rank.replace({first, 1});
  roots.insert(first);
  disjointSets.replace({first, ics});
  concretization = ics->concretization;
}

void IndependentConstraintSetUnion::addIndependentConstraintSetUnion(
    const IndependentConstraintSetUnion &icsu) {
  add(icsu);
  concretization.addIndependentAssignment(icsu.concretization);
}

void IndependentConstraintSetUnion::updateConcretization(
    const Assignment &delta) {
  for (ref<Expr> e : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(e);
    Assignment part = delta.part(ics->getSymcretes());
    ics = ics->updateConcretization(part, concretizedExprs);
    disjointSets.replace({e, ics});
  }
  for (auto it : delta.bindings) {
    concretization.bindings.replace({it.first, it.second});
  }
}

void IndependentConstraintSetUnion::removeConcretization(
    const Assignment &remove) {
  for (ref<Expr> e : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(e);
    Assignment part = remove.part(ics->getSymcretes());
    ics = ics->removeConcretization(part, concretizedExprs);
    disjointSets.replace({e, ics});
  }
  for (auto it : remove.bindings) {
    concretization.bindings.remove(it.first);
  }
}

void IndependentConstraintSetUnion::reEvaluateConcretization(
    const Assignment &newConcretization) {
  Assignment delta;
  Assignment removed;
  for (const auto it : concretization) {
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
    ref<Expr> e,
    std::vector<ref<const IndependentConstraintSet>> &result) const {
  ref<const IndependentConstraintSet> compare = new IndependentConstraintSet(e);
  for (ref<Expr> r : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(r);
    if (!IndependentConstraintSet::intersects(ics, compare)) {
      result.push_back(ics);
    }
  }
}

void IndependentConstraintSetUnion::getAllDependentConstraintSets(
    ref<Expr> e,
    std::vector<ref<const IndependentConstraintSet>> &result) const {
  ref<const IndependentConstraintSet> compare = new IndependentConstraintSet(e);
  for (ref<Expr> r : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(r);
    if (IndependentConstraintSet::intersects(ics, compare)) {
      result.push_back(ics);
    }
  }
}

void IndependentConstraintSetUnion::addExpr(ref<Expr> e) {
  addValue(e);
  disjointSets.replace({find(e), disjointSets.at(find(e))->addExpr(e)});
}

void IndependentConstraintSetUnion::addSymcrete(ref<Symcrete> s) {
  ref<Expr> value = s->symcretized;
  if (internalStorage.find(value) != internalStorage.end()) {
    return;
  }
  parent.insert({value, value});
  roots.insert(value);
  rank.insert({value, 0});
  disjointSets.insert({value, new IndependentConstraintSet(s)});

  internalStorage.insert(value);
  internalStorage_ty oldRoots = roots;
  for (ref<Expr> v : oldRoots) {
    if (!areJoined(v, value) &&
        IndependentConstraintSet::intersects(disjointSets.at(find(v)),
                                             disjointSets.at(find(value)))) {
      merge(v, value);
    }
  }
  disjointSets.replace(
      {find(value), disjointSets.at(find(value))->addExpr(value)});
}

IndependentConstraintSetUnion
IndependentConstraintSetUnion::getConcretizedVersion() const {
  IndependentConstraintSetUnion icsu;
  for (ref<Expr> i : roots) {
    ref<const IndependentConstraintSet> root = disjointSets.at(i);
    icsu.add(root->concretizedSets);
    icsu.concretization.addIndependentAssignment(root->concretization);
  }
  icsu.concretizedExprs = concretizedExprs;
  return icsu;
}

IndependentConstraintSetUnion
IndependentConstraintSetUnion::getConcretizedVersion(
    const Assignment &newConcretization) const {
  IndependentConstraintSetUnion icsu = *this;
  icsu.reEvaluateConcretization(newConcretization);
  return icsu.getConcretizedVersion();
}
} // namespace klee
