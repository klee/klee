//===-- Assignment.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_ASSIGNMENT_H
#define KLEE_ASSIGNMENT_H

#include "klee/ADT/PersistentMap.h"
#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/ExprEvaluator.h"

#include <map>
#include <set>

namespace klee {
class Array;
class Symcrete;
struct SymcreteLess;
class ConstraintSet;

typedef std::set<ref<Symcrete>, SymcreteLess> SymcreteOrderedSet;
using symcretes_ty = SymcreteOrderedSet;

class Assignment {
public:
  using bindings_ty =
      PersistentMap<const Array *, SparseStorage<unsigned char>>;

  bindings_ty bindings;

  friend bool operator==(const Assignment &lhs, const Assignment &rhs) {
    return lhs.bindings == rhs.bindings;
  }

public:
  Assignment() {}
  Assignment(const bindings_ty &_bindings) : bindings(_bindings) {}
  Assignment(const std::vector<const Array *> &objects,
             const std::vector<SparseStorage<unsigned char>> &values) {
    assert(objects.size() == values.size());
    for (unsigned i = 0; i < values.size(); ++i) {
      const Array *os = objects.at(i);
      const SparseStorage<unsigned char> &arr = values.at(i);
      bindings.insert(std::make_pair(os, arr));
    }
  }

  void addIndependentAssignment(const Assignment &b);

  ref<Expr> evaluate(const Array *mo, unsigned index,
                     bool allowFreeValues = true) const;
  ref<Expr> evaluate(ref<Expr> e, bool allowFreeValues = true) const;
  constraints_ty createConstraintsFromAssignment() const;

  template <typename InputIterator>
  bool satisfies(InputIterator begin, InputIterator end,
                 bool allowFreeValues = true);
  template <typename InputIterator>
  bool satisfiesNonBoolean(InputIterator begin, InputIterator end,
                           bool allowFreeValues = true);
  void dump() const;

  Assignment diffWith(const Assignment &other) const;
  Assignment part(const SymcreteOrderedSet &symcretes) const;

  bindings_ty::iterator begin() const { return bindings.begin(); }
  bindings_ty::iterator end() const { return bindings.end(); }
  bool isEmpty() { return begin() == end(); }

  std::vector<const Array *> keys() const;
  std::vector<SparseStorage<unsigned char>> values() const;
};

class AssignmentEvaluator : public ExprEvaluator {
  const Assignment &a;
  bool allowFreeValues;

protected:
  ref<Expr> getInitialValue(const Array &mo, unsigned index) {
    return a.evaluate(&mo, index, allowFreeValues);
  }

public:
  AssignmentEvaluator(const Assignment &_a, bool _allowFreeValues)
      : a(_a), allowFreeValues(_allowFreeValues) {}
};

/***/

inline ref<Expr> Assignment::evaluate(const Array *array, unsigned index,
                                      bool allowFreeValues) const {
  assert(array);
  auto sizeExpr = evaluate(array->size);
  bindings_ty::iterator it = bindings.find(array);
  if (it != bindings.end() && isa<ConstantExpr>(sizeExpr) &&
      index < cast<ConstantExpr>(sizeExpr)->getZExtValue()) {
    return ConstantExpr::alloc(it->second.load(index), array->getRange());
  } else {
    if (allowFreeValues) {
      return ReadExpr::create(UpdateList(array, ref<UpdateNode>(nullptr)),
                              ConstantExpr::alloc(index, array->getDomain()));
    } else {
      return ConstantExpr::alloc(0, array->getRange());
    }
  }
}

inline ref<Expr> Assignment::evaluate(ref<Expr> e, bool allowFreeValues) const {
  AssignmentEvaluator v(*this, allowFreeValues);
  return v.visit(e);
}

template <typename InputIterator>
inline bool Assignment::satisfies(InputIterator begin, InputIterator end,
                                  bool allowFreeValues) {
  AssignmentEvaluator v(*this, allowFreeValues);
  for (; begin != end; ++begin) {
    assert((*begin)->getWidth() == Expr::Bool && "constraints must be boolean");
    if (!v.visit(*begin)->isTrue())
      return false;
  }
  return true;
}

template <typename InputIterator>
inline bool Assignment::satisfiesNonBoolean(InputIterator begin,
                                            InputIterator end,
                                            bool allowFreeValues) {
  AssignmentEvaluator v(*this, allowFreeValues);
  for (; begin != end; ++begin) {
    if (!isa<ConstantExpr>(v.visit(*begin)))
      return false;
  }
  return true;
}
} // namespace klee

#endif /* KLEE_ASSIGNMENT_H */
