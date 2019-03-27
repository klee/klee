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

// FIXME: Currently we use ConstraintManager for two things: to pass
// sets of constraints around, and to optimize constraints. We should
// move the first usage into a separate data structure
// (ConstraintSet?) which ConstraintManager could embed if it likes.
namespace klee {

class ExprVisitor;

class ConstraintManager {
public:
  using constraints_ty = std::vector<ref<Expr>>;
  using iterator = constraints_ty::iterator;
  using const_iterator = constraints_ty::const_iterator;

  ConstraintManager() = default;
  ConstraintManager(const ConstraintManager &cs) = default;
  ConstraintManager &operator=(const ConstraintManager &cs) = default;
  ConstraintManager(ConstraintManager &&cs) = default;
  ConstraintManager &operator=(ConstraintManager &&cs) = default;

  // create from constraints with no optimization
  explicit ConstraintManager(const std::vector<ref<Expr>> &_constraints);

  typedef std::vector< ref<Expr> >::const_iterator constraint_iterator;

  ref<Expr> simplifyExpr(ref<Expr> e) const;

  void addConstraint(ref<Expr> e);

  bool empty() const noexcept ;
  ref<Expr> back() const;
  constraint_iterator begin() const;
  constraint_iterator end() const;
  size_t size() const noexcept ;

  bool operator==(const ConstraintManager &other) const;

private:
  std::vector<ref<Expr>> constraints;

  // returns true iff the constraints were modified
  bool rewriteConstraints(ExprVisitor &visitor);

  void addConstraintInternal(ref<Expr> e);
};

} // namespace klee

#endif /* KLEE_CONSTRAINTS_H */
