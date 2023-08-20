#ifndef KLEE_INDEPENDENTSET_H
#define KLEE_INDEPENDENTSET_H

#include "klee/ADT/DisjointSetUnion.h"
#include "klee/ADT/PersistentMap.h"
#include "klee/ADT/PersistentSet.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/Symcrete.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <map>
#include <set>
#include <string>
#include <vector>

namespace klee {

template <class T> class DenseSet {
  typedef std::set<T> set_ty;
  set_ty s;

public:
  DenseSet() {}

  void add(T x) { s.insert(x); }
  void add(T start, T end) {
    for (; start < end; start++)
      s.insert(start);
  }

  // returns true iff set is changed by addition
  bool add(const DenseSet &b) {
    bool modified = false;
    for (typename set_ty::const_iterator it = b.s.begin(), ie = b.s.end();
         it != ie; ++it) {
      if (modified || !s.count(*it)) {
        modified = true;
        s.insert(*it);
      }
    }
    return modified;
  }

  bool intersects(const DenseSet &b) const {
    for (typename set_ty::iterator it = s.begin(), ie = s.end(); it != ie; ++it)
      if (b.s.count(*it))
        return true;
    return false;
  }

  std::set<unsigned>::iterator begin() { return s.begin(); }

  std::set<unsigned>::iterator end() { return s.end(); }

  void print(llvm::raw_ostream &os) const {
    bool first = true;
    os << "{";
    for (typename set_ty::iterator it = s.begin(), ie = s.end(); it != ie;
         ++it) {
      if (first) {
        first = false;
      } else {
        os << ",";
      }
      os << *it;
    }
    os << "}";
  }
};

template <class T>
inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const DenseSet<T> &dis) {
  dis.print(os);
  return os;
}

class IndependentConstraintSet {
private:
  using InnerSetUnion = DisjointSetUnion<ref<Expr>, IndependentConstraintSet>;

public:
  // All containers need to become persistent to make fast copy and faster
  // merge possible map from concretized to normal
  typedef PersistentMap<const Array *, DenseSet<unsigned>> elements_ty;
  elements_ty
      elements; // Represents individual elements of array accesses (arr[1])
  PersistentSet<const Array *>
      wholeObjects; // Represents symbolically accessed arrays (arr[x])
  PersistentSet<ref<Expr>> exprs; // All expressions (constraints) that are
                                  // associated with this factor
  PersistentSet<ref<Symcrete>>
      symcretes; // All symcretes associated with this factor

  Assignment concretization;

  InnerSetUnion concretizedSets;

  ref<const IndependentConstraintSet> addExpr(ref<Expr> e) const;
  ref<const IndependentConstraintSet>
  updateConcretization(const Assignment &delta,
                       ExprHashMap<ref<Expr>> &changedExprs) const;
  ref<const IndependentConstraintSet>
  removeConcretization(const Assignment &delta,
                       ExprHashMap<ref<Expr>> &changedExprs) const;

  void
  addValuesToAssignment(const std::vector<const Array *> &objects,
                        const std::vector<SparseStorage<unsigned char>> &values,
                        Assignment &assign) const;

  IndependentConstraintSet();
  IndependentConstraintSet(ref<Expr> e);
  IndependentConstraintSet(ref<Symcrete> s);
  IndependentConstraintSet(const ref<const IndependentConstraintSet> &ics);

  IndependentConstraintSet &operator=(const IndependentConstraintSet &ies);

  void print(llvm::raw_ostream &os) const;

  static bool intersects(ref<const IndependentConstraintSet> a,
                         ref<const IndependentConstraintSet> b);

  static ref<const IndependentConstraintSet>
  merge(ref<const IndependentConstraintSet> a,
        ref<const IndependentConstraintSet> b);

  // Extracts which arrays are referenced from a particular independent set.
  // Examines both the actual known array accesses arr[1] plus the undetermined
  // accesses arr[x].Z
  void calculateArrayReferences(std::vector<const Array *> &returnVector) const;

  SymcreteOrderedSet getSymcretes() const {
    SymcreteOrderedSet a;
    for (ref<Symcrete> s : symcretes) {
      a.insert(s);
    }
    return a;
  }

  constraints_ty getConstraints() const {
    constraints_ty a;
    for (ref<Expr> e : exprs) {
      a.insert(e);
    }
    return a;
  }
  mutable class ReferenceCounter _refCount;
};

void calculateArraysInFactors(
    const std::vector<ref<const IndependentConstraintSet>> &factors,
    ref<Expr> queryExpr, std::vector<const Array *> &returnVector);

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const IndependentConstraintSet &ies) {
  ies.print(os);
  return os;
}

} // namespace klee

#endif /* KLEE_INDEPENDENTSET_H */
