#include "klee/Expr/IndependentSet.h"

#include "klee/ADT/Ref.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Expr/Symcrete.h"
#include "klee/Solver/Solver.h"

#include <list>
#include <map>
#include <queue>
#include <set>
#include <vector>

namespace klee {
ref<const IndependentConstraintSet>
IndependentConstraintSet::updateConcretization(
    const Assignment &delta, ExprHashMap<ref<Expr>> &concretizedExprs) const {
  ref<IndependentConstraintSet> ics = new IndependentConstraintSet(*this);
  if (delta.bindings.size() == 0) {
    return ics;
  }
  for (auto &i : delta.bindings) {
    ics->concretization.bindings.replace({i.first, i.second});
  }
  InnerSetUnion DSU;
  for (ref<Expr> i : exprs) {
    ref<Expr> e = ics->concretization.evaluate(i);
    if (auto ce = dyn_cast<ConstantExpr>(e)) {
      assert(ce->isTrue() && "Attempt to add invalid constraint");
      continue;
    }
    concretizedExprs[i] = e;
    DSU.addValue(new ExprEitherSymcrete::left(e));
  }
  for (ref<Symcrete> s : symcretes) {
    ref<Expr> e = EqExpr::create(ics->concretization.evaluate(s->symcretized),
                                 s->symcretized);
    if (auto ce = dyn_cast<ConstantExpr>(e)) {
      assert(ce->isTrue() && "Attempt to add invalid constraint");
      continue;
    }
    DSU.addValue(new ExprEitherSymcrete::left(e));
  }
  auto concretizationConstraints =
      ics->concretization.createConstraintsFromAssignment();
  for (ref<Expr> e : concretizationConstraints) {
    if (auto ce = dyn_cast<ConstantExpr>(e)) {
      assert(ce->isTrue() && "Attempt to add invalid constraint");
      continue;
    }
    DSU.addValue(new ExprEitherSymcrete::left(e));
  }
  ics->concretizedSets = DSU;
  return ics;
}

ref<const IndependentConstraintSet>
IndependentConstraintSet::removeConcretization(
    const Assignment &delta, ExprHashMap<ref<Expr>> &concretizedExprs) const {
  ref<IndependentConstraintSet> ics = new IndependentConstraintSet(*this);
  if (delta.bindings.size() == 0) {
    return ics;
  }
  for (auto &i : delta.bindings) {
    ics->concretization.bindings.remove(i.first);
  }
  InnerSetUnion DSU;
  for (ref<Expr> i : exprs) {
    ref<Expr> e = ics->concretization.evaluate(i);
    if (auto ce = dyn_cast<ConstantExpr>(e)) {
      assert(ce->isTrue() && "Attempt to add invalid constraint");
      continue;
    }
    concretizedExprs[i] = e;
    DSU.addValue(new ExprEitherSymcrete::left(e));
  }
  for (ref<Symcrete> s : symcretes) {
    ref<Expr> e = EqExpr::create(ics->concretization.evaluate(s->symcretized),
                                 s->symcretized);
    if (auto ce = dyn_cast<ConstantExpr>(e)) {
      assert(ce->isTrue() && "Attempt to add invalid constraint");
      continue;
    }
    DSU.addValue(new ExprEitherSymcrete::left(e));
  }
  auto concretizationConstraints =
      ics->concretization.createConstraintsFromAssignment();
  for (ref<Expr> e : concretizationConstraints) {
    if (auto ce = dyn_cast<ConstantExpr>(e)) {
      assert(ce->isTrue() && "Attempt to add invalid constraint");
      continue;
    }
    DSU.addValue(new ExprEitherSymcrete::left(e));
  }

  ics->concretizedSets = DSU;
  return ics;
}

void IndependentConstraintSet::addValuesToAssignment(
    const std::vector<const Array *> &objects,
    const std::vector<SparseStorage<unsigned char>> &values,
    Assignment &assign) const {
  for (unsigned i = 0; i < objects.size(); i++) {
    if (assign.bindings.count(objects[i])) {
      SparseStorage<unsigned char> value = assign.bindings.at(objects[i]);
      DenseSet<unsigned> ds = (elements.find(objects[i]))->second;
      for (std::set<unsigned>::iterator it2 = ds.begin(); it2 != ds.end();
           it2++) {
        unsigned index = *it2;
        value.store(index, values[i].load(index));
      }
      assign.bindings.replace({objects[i], value});
    } else {
      assign.bindings.replace({objects[i], values[i]});
    }
  }
}

IndependentConstraintSet::IndependentConstraintSet() {}

IndependentConstraintSet::IndependentConstraintSet(ref<ExprEitherSymcrete> v) {
  if (isa<ExprEitherSymcrete::left>(v)) {
    initIndependentConstraintSet(cast<ExprEitherSymcrete::left>(v)->value());
  } else {
    initIndependentConstraintSet(cast<ExprEitherSymcrete::right>(v)->value());
  }
}

void IndependentConstraintSet::initIndependentConstraintSet(ref<Expr> e) {
  exprs.insert(e);
  // Track all reads in the program.  Determines whether reads are
  // concrete or symbolic.  If they are symbolic, "collapses" array
  // by adding it to wholeObjects.  Otherwise, creates a mapping of
  // the form Map<array, set<index>> which tracks which parts of the
  // array are being accessed.
  std::vector<ref<ReadExpr>> reads;
  findReads(e, /* visitUpdates= */ true, reads);
  for (unsigned i = 0; i != reads.size(); ++i) {
    ReadExpr *re = reads[i].get();
    const Array *array = re->updates.root;

    // Reads of a constant array don't alias.
    if (re->updates.root->isConstantArray() && !re->updates.head)
      continue;

    if (!wholeObjects.count(array)) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(re->index)) {
        // if index constant, then add to set of constraints operating
        // on that array (actually, don't add constraint, just set index)
        DenseSet<unsigned> dis;
        if (elements.find(array) != elements.end()) {
          dis = (elements.find(array))->second;
        }
        dis.add((unsigned)CE->getZExtValue(32));
        elements.replace({array, dis});
      } else {
        elements_ty::iterator it2 = elements.find(array);
        if (it2 != elements.end())
          elements.remove(it2->first);
        wholeObjects.insert(array);
      }
    }
  }
}

void IndependentConstraintSet::initIndependentConstraintSet(ref<Symcrete> s) {
  symcretes.insert(s);

  // Track all reads in the program.  Determines whether reads are
  // concrete or symbolic.  If they are symbolic, "collapses" array
  // by adding it to wholeObjects.  Otherwise, creates a mapping of
  // the form Map<array, set<index>> which tracks which parts of the
  // array are being accessed.
  std::vector<ref<ReadExpr>> reads;

  SymcreteOrderedSet usedSymcretes;
  usedSymcretes.insert(s);
  std::queue<ref<Symcrete>> queueSymcretes;
  queueSymcretes.push(s);

  while (!queueSymcretes.empty()) {
    ref<Symcrete> top = queueSymcretes.front();
    queueSymcretes.pop();
    findReads(top->symcretized, true, reads);
    for (Symcrete &dependentSymcrete : top->dependentSymcretes()) {
      if (usedSymcretes.insert(ref<Symcrete>(&dependentSymcrete)).second) {
        queueSymcretes.push(ref<Symcrete>(&dependentSymcrete));
      }
    }
  }

  for (unsigned i = 0; i != reads.size(); ++i) {
    ReadExpr *re = reads[i].get();
    const Array *array = re->updates.root;

    // Reads of a constant array don't alias.
    if (re->updates.root->isConstantArray() && !re->updates.head)
      continue;

    if (!wholeObjects.count(array)) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(re->index)) {
        // if index constant, then add to set of constraints operating
        // on that array (actually, don't add constraint, just set index)
        DenseSet<unsigned> dis;
        if (elements.find(array) != elements.end()) {
          dis = (elements.find(array))->second;
        }
        dis.add((unsigned)CE->getZExtValue(32));
        elements.replace({array, dis});
      } else {
        elements_ty::iterator it2 = elements.find(array);
        if (it2 != elements.end())
          elements.remove(it2->first);
        wholeObjects.insert(array);
      }
    }
  }
}

IndependentConstraintSet::IndependentConstraintSet(
    const IndependentConstraintSet &ics)
    : elements(ics.elements), wholeObjects(ics.wholeObjects), exprs(ics.exprs),
      symcretes(ics.symcretes), concretization(ics.concretization),
      concretizedSets(ics.concretizedSets) {}

void IndependentConstraintSet::print(llvm::raw_ostream &os) const {
  os << "{";
  bool first = true;
  for (PersistentSet<const Array *>::iterator it = wholeObjects.begin(),
                                              ie = wholeObjects.end();
       it != ie; ++it) {
    const Array *array = *it;

    if (first) {
      first = false;
    } else {
      os << ", ";
    }

    os << "MO" << array->getIdentifier();
  }
  for (elements_ty::iterator it = elements.begin(), ie = elements.end();
       it != ie; ++it) {
    const Array *array = it->first;
    const DenseSet<unsigned> &dis = it->second;

    if (first) {
      first = false;
    } else {
      os << ", ";
    }

    os << "MO" << array->getIdentifier() << " : " << dis;
  }
  os << "}";
}

bool IndependentConstraintSet::intersects(
    ref<const IndependentConstraintSet> a,
    ref<const IndependentConstraintSet> b) {
  if (a->wholeObjects.size() + a->elements.size() >
      b->wholeObjects.size() + b->elements.size()) {
    std::swap(a, b);
  }
  // If there are any symbolic arrays in our query that b accesses
  for (PersistentSet<const Array *>::iterator it = a->wholeObjects.begin(),
                                              ie = a->wholeObjects.end();
       it != ie; ++it) {
    const Array *array = *it;
    if (b->wholeObjects.count(array) ||
        b->elements.find(array) != b->elements.end())
      return true;
  }
  for (elements_ty::iterator it = a->elements.begin(), ie = a->elements.end();
       it != ie; ++it) {
    const Array *array = it->first;
    // if the array we access is symbolic in b
    if (b->wholeObjects.count(array))
      return true;
    elements_ty::iterator it2 = b->elements.find(array);
    // if any of the elements we access are also accessed by b
    if (it2 != b->elements.end()) {
      if (it->second.intersects(it2->second)) {
        return true;
      }
    }
  }
  // No need to check symcretes here, arrays must be sufficient.
  return false;
}

ref<const IndependentConstraintSet>
IndependentConstraintSet::merge(ref<const IndependentConstraintSet> A,
                                ref<const IndependentConstraintSet> B) {
  ref<IndependentConstraintSet> a = new IndependentConstraintSet(*A);
  ref<IndependentConstraintSet> b = new IndependentConstraintSet(*B);

  if (a->wholeObjects.size() + a->elements.size() <
      b->wholeObjects.size() + b->elements.size()) {
    std::swap(a, b);
  }
  for (ref<Expr> expr : b->exprs) {
    a->exprs.insert(expr);
  }
  for (const ref<Symcrete> &symcrete : b->symcretes) {
    a->symcretes.insert(symcrete);
  }

  for (PersistentSet<const Array *>::iterator it = b->wholeObjects.begin(),
                                              ie = b->wholeObjects.end();
       it != ie; ++it) {
    const Array *array = *it;
    elements_ty::iterator it2 = a->elements.find(array);
    if (it2 != a->elements.end()) {
      a->elements.remove(it2->first);
      a->wholeObjects.insert(array);
    } else {
      if (!a->wholeObjects.count(array)) {
        a->wholeObjects.insert(array);
      }
    }
  }
  for (elements_ty::iterator it = b->elements.begin(), ie = b->elements.end();
       it != ie; ++it) {
    const Array *array = it->first;
    if (!a->wholeObjects.count(array)) {
      elements_ty::iterator it2 = a->elements.find(array);
      if (it2 == a->elements.end()) {
        a->elements.insert(*it);
      } else {
        DenseSet<unsigned> dis = it2->second;
        dis.add(it->second);
        a->elements.replace({array, dis});
      }
    }
  }
  b->addValuesToAssignment(b->concretization.keys(), b->concretization.values(),
                           a->concretization);

  if (!a->concretization.bindings.empty()) {
    InnerSetUnion DSU;
    for (ref<Expr> i : a->exprs) {
      ref<Expr> e = a->concretization.evaluate(i);
      if (auto ce = dyn_cast<ConstantExpr>(e)) {
        assert(ce->isTrue() && "Attempt to add invalid constraint");
        continue;
      }
      DSU.addValue(new ExprEitherSymcrete::left(e));
    }
    for (ref<Symcrete> s : a->symcretes) {
      ref<Expr> e = EqExpr::create(a->concretization.evaluate(s->symcretized),
                                   s->symcretized);
      if (auto ce = dyn_cast<ConstantExpr>(e)) {
        assert(ce->isTrue() && "Attempt to add invalid constraint");
        continue;
      }
      DSU.addValue(new ExprEitherSymcrete::left(e));
    }
    auto concretizationConstraints =
        a->concretization.createConstraintsFromAssignment();
    for (ref<Expr> e : concretizationConstraints) {
      if (auto ce = dyn_cast<ConstantExpr>(e)) {
        assert(ce->isTrue() && "Attempt to add invalid constraint");
        continue;
      }
      DSU.addValue(new ExprEitherSymcrete::left(e));
    }

    a->concretizedSets = DSU;
  }

  return a;
}

void IndependentConstraintSet::calculateArrayReferences(
    std::vector<const Array *> &returnVector) const {
  std::set<const Array *> thisSeen;
  for (IndependentConstraintSet::elements_ty::iterator it = elements.begin();
       it != elements.end(); ++it) {
    thisSeen.insert(it->first);
  }
  for (PersistentSet<const Array *>::iterator it = wholeObjects.begin();
       it != wholeObjects.end(); ++it) {
    thisSeen.insert(*it);
  }
  for (std::set<const Array *>::iterator it = thisSeen.begin();
       it != thisSeen.end(); ++it) {
    returnVector.push_back(*it);
  }
}

void calculateArraysInFactors(
    const std::vector<ref<const IndependentConstraintSet>> &factors,
    ref<Expr> queryExpr, std::vector<const Array *> &returnVector) {
  std::set<const Array *> returnSet;
  for (ref<const IndependentConstraintSet> ics : factors) {
    std::vector<const Array *> result;
    ics->calculateArrayReferences(result);
    returnSet.insert(result.begin(), result.end());
  }
  std::vector<const Array *> result;
  ref<IndependentConstraintSet> queryExprSet =
      new IndependentConstraintSet(new ExprEitherSymcrete::left(queryExpr));
  queryExprSet->calculateArrayReferences(result);
  returnSet.insert(result.begin(), result.end());
  returnVector.insert(returnVector.begin(), returnSet.begin(), returnSet.end());
}

} // namespace klee
