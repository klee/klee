//===-- Constraints.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CONSTRAINTS_H
#define KLEE_CONSTRAINTS_H

#include "klee/Expr/Expr.h"

namespace klee {

/// Resembles a set of constraints that can be passed around
///
class ConstraintSet {
  friend class ConstraintManager;

public:
  using constraints_ty = std::vector<ref<Expr>>;
  using iterator = constraints_ty::iterator;
  using const_iterator = constraints_ty::const_iterator;

  typedef const_iterator constraint_iterator;

  bool empty() const;
  constraint_iterator begin() const;
  constraint_iterator end() const;
  size_t size() const noexcept ;

  explicit ConstraintSet(constraints_ty cs) : constraints(std::move(cs)) {}
  ConstraintSet() = default;

  void push_back(const ref<Expr> &e);

  bool operator==(const ConstraintSet &b) const {
    return constraints == b.constraints;
  }

private:
  constraints_ty constraints;
};

class ExprVisitor;

/// Manages constraints, e.g. optimisation
class ConstraintManager {
public:
  // create from constraints with no optimization
  explicit ConstraintManager(ConstraintSet &constraints);

  static ref<Expr> simplifyExpr(const ConstraintSet &constraints,
                                const ref<Expr> &e);

  void addConstraint(ref<Expr> e);

  // returns true iff the constraints were modified
  bool rewriteConstraints(ExprVisitor &visitor);

  void addConstraintInternal(ref<Expr> e);

private:
  ConstraintSet &constraints;
};

} // namespace klee

#endif /* KLEE_CONSTRAINTS_H */