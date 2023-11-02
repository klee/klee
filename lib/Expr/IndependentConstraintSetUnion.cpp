#include "klee/Expr/IndependentConstraintSetUnion.h"

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
    auto v = ref<ExprEitherSymcrete::left>(new ExprEitherSymcrete::left(e));
    rank.insert_or_assign(v, 0);
    internalStorage.insert(v);
  }

  for (ref<Symcrete> s : ics->symcretes) {
    auto v = ref<ExprEitherSymcrete::right>(new ExprEitherSymcrete::right(s));
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
}

void IndependentConstraintSetUnion::updateConcretization(
    const Assignment &delta) {
  for (auto &e : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(e);
    Assignment part = delta.part(ics->getSymcretes());
    ics = ics->updateConcretization(part, concretizedExprs);
    disjointSets.insert_or_assign(e, ics);
  }
  for (auto &it : delta.bindings) {
    concretization.bindings.replace({it.first, it.second});
  }
}

void IndependentConstraintSetUnion::removeConcretization(
    const Assignment &remove) {
  for (auto &e : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(e);
    Assignment part = remove.part(ics->getSymcretes());
    ics = ics->removeConcretization(part, concretizedExprs);
    disjointSets.insert_or_assign(e, ics);
  }
  for (auto &it : remove.bindings) {
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
    ref<Expr> e,
    std::vector<ref<const IndependentConstraintSet>> &result) const {
  ref<const IndependentConstraintSet> compare =
      new IndependentConstraintSet(new ExprEitherSymcrete::left(e));
  for (auto &r : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(r);
    if (!IndependentConstraintSet::intersects(ics, compare)) {
      result.push_back(ics);
    }
  }
}

void IndependentConstraintSetUnion::getAllDependentConstraintSets(
    ref<Expr> e,
    std::vector<ref<const IndependentConstraintSet>> &result) const {
  ref<const IndependentConstraintSet> compare =
      new IndependentConstraintSet(new ExprEitherSymcrete::left(e));
  for (auto &r : roots) {
    ref<const IndependentConstraintSet> ics = disjointSets.at(r);
    if (IndependentConstraintSet::intersects(ics, compare)) {
      result.push_back(ics);
    }
  }
}

void IndependentConstraintSetUnion::addExpr(ref<Expr> e) {
  addValue(new ExprEitherSymcrete::left(e));
}

void IndependentConstraintSetUnion::addSymcrete(ref<Symcrete> s) {
  addValue(new ExprEitherSymcrete::right(s));
}

IndependentConstraintSetUnion
IndependentConstraintSetUnion::getConcretizedVersion() const {
  IndependentConstraintSetUnion icsu;
  for (auto &i : roots) {
    ref<const IndependentConstraintSet> root = disjointSets.at(i);
    if (root->concretization.bindings.empty()) {
      for (ref<Expr> expr : root->exprs) {
        icsu.addValue(new ExprEitherSymcrete::left(expr));
      }
    } else {
      icsu.add(root->concretizedSets);
    }
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
