#ifndef KLEE_INDEPENDENTSET_H
#define KLEE_INDEPENDENTSET_H

#include "klee/Expr/Expr.h"
#include "klee/Solver/Solver.h"
#include "llvm/Support/raw_ostream.h"
#include <list>
#include <set>

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

  bool intersects(const DenseSet &b) {
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

class IndependentElementSet {
public:
  typedef std::map<const Array *, DenseSet<unsigned>> elements_ty;
  elements_ty
      elements; // Represents individual elements of array accesses (arr[1])
  std::set<const Array *>
      wholeObjects; // Represents symbolically accessed arrays (arr[x])
  std::vector<ref<Expr>> exprs; // All expressions that are associated with this
                                // factor Although order doesn't matter, we use
                                // a vector to match the ConstraintManager
                                // constructor that will eventually be invoked.

  IndependentElementSet();
  IndependentElementSet(ref<Expr> e);
  IndependentElementSet(const IndependentElementSet &ies);

  IndependentElementSet &operator=(const IndependentElementSet &ies);

  void print(llvm::raw_ostream &os) const;

  // more efficient when this is the smaller set
  bool intersects(const IndependentElementSet &b);

  // returns true iff set is changed by addition
  bool add(const IndependentElementSet &b);
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const IndependentElementSet &ies) {
  ies.print(os);
  return os;
}

// Breaks down a constraint into all of it's individual pieces, returning a
// list of IndependentElementSets or the independent factors.
//
// Caller takes ownership of returned std::list.
std::list<IndependentElementSet> *
getAllIndependentConstraintsSets(const Query &query);

IndependentElementSet getIndependentConstraints(const Query &query,
                                                std::vector<ref<Expr>> &result);

// Extracts which arrays are referenced from a particular independent set.
// Examines both the actual known array accesses arr[1] plus the undetermined
// accesses arr[x].
void calculateArrayReferences(const IndependentElementSet &ie,
                              std::vector<const Array *> &returnVector);
} // namespace klee

#endif /* KLEE_INDEPENDENTSET_H */
