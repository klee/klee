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

#include "klee/ADT/Ref.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentConstraintSetUnion.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Expr/Path.h"
#include "klee/Expr/Symcrete.h"

#include <set>
#include <vector>

namespace klee {

enum class RewriteEqualitiesPolicy { None, Simple, Full };

class MemoryObject;
struct KInstruction;

/// Resembles a set of constraints that can be passed around
///
class ConstraintSet {
public:
  ConstraintSet(constraints_ty cs, symcretes_ty symcretes,
                Assignment concretization);
  ConstraintSet(ref<const IndependentConstraintSet> ics);
  ConstraintSet(const std::vector<ref<const IndependentConstraintSet>> &ics);
  ConstraintSet();

  void addConstraint(ref<Expr> e, const Assignment &delta);
  void addSymcrete(ref<Symcrete> s, const Assignment &concretization);
  bool isSymcretized(ref<Expr> expr) const;

  void rewriteConcretization(const Assignment &a);
  ConstraintSet withExpr(ref<Expr> e) const;

  std::vector<const Array *> gatherArrays() const;
  std::vector<const Array *> gatherSymcretizedArrays() const;

  bool operator==(const ConstraintSet &b) const {
    return _constraints == b._constraints && _symcretes == b._symcretes;
  }

  bool operator<(const ConstraintSet &b) const {
    return _constraints < b._constraints ||
           (_constraints == b._constraints && _symcretes < b._symcretes);
  }
  ConstraintSet getConcretizedVersion() const;
  ConstraintSet getConcretizedVersion(const Assignment &c) const;
  void dump() const;
  void print(llvm::raw_ostream &os) const;

  void changeCS(constraints_ty &cs);

  const constraints_ty &cs() const;
  const symcretes_ty &symcretes() const;
  const Assignment &concretization() const;
  const IndependentConstraintSetUnion &independentElements() const;

  void getAllIndependentConstraintsSets(
      ref<Expr> queryExpr,
      std::vector<ref<const IndependentConstraintSet>> &result) const;

  void getAllDependentConstraintsSets(
      ref<Expr> queryExpr,
      std::vector<ref<const IndependentConstraintSet>> &result) const;

private:
  constraints_ty _constraints;
  symcretes_ty _symcretes;
  Assignment _concretization;
  IndependentConstraintSetUnion _independentElements;
};

class PathConstraints {
public:
  using ordered_constraints_ty =
      std::map<Path::PathIndex, constraints_ty, Path::PathIndexCompare>;

  void advancePath(KInstruction *ki);
  void advancePath(const Path &path);
  ExprHashSet addConstraint(ref<Expr> e, const Assignment &delta,
                            Path::PathIndex currIndex);
  ExprHashSet addConstraint(ref<Expr> e, const Assignment &delta);
  bool isSymcretized(ref<Expr> expr) const;
  void addSymcrete(ref<Symcrete> s, const Assignment &concretization);
  void rewriteConcretization(const Assignment &a);

  const constraints_ty &original() const;
  const ExprHashMap<ExprHashSet> &simplificationMap() const;
  const ConstraintSet &cs() const;
  const Path &path() const;
  const ExprHashMap<Path::PathIndex> &indexes() const;
  const ordered_constraints_ty &orderedCS() const;

  static PathConstraints concat(const PathConstraints &l,
                                const PathConstraints &r);

private:
  Path _path;
  constraints_ty _original;
  ConstraintSet constraints;
  ExprHashMap<Path::PathIndex> pathIndexes;
  ordered_constraints_ty orderedConstraints;
  ExprHashMap<ExprHashSet> _simplificationMap;
};

struct Conflict {
  Path path;
  constraints_ty core;
  Conflict() = default;
};

struct TargetedConflict {
  friend class ref<TargetedConflict>;

private:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

public:
  Conflict conflict;
  KBlock *target;

  TargetedConflict(Conflict &_conflict, KBlock *_target)
      : conflict(_conflict), target(_target) {}
};

class Simplificator {
public:
  struct ExprResult {
    ref<Expr> simplified;
    ExprHashSet dependency;
  };

  struct SetResult {
    constraints_ty simplified;
    ExprHashMap<ExprHashSet> dependency;
  };

public:
  static ExprResult simplifyExpr(const constraints_ty &constraints,
                                 const ref<Expr> &expr);

  static ExprResult simplifyExpr(const ConstraintSet &constraints,
                                 const ref<Expr> &expr);

  static Simplificator::SetResult
  simplify(const constraints_ty &constraints,
           RewriteEqualitiesPolicy p = RewriteEqualitiesPolicy::Full);

  static ExprHashMap<ExprHashSet>
  composeExprDependencies(const ExprHashMap<ExprHashSet> &upper,
                          const ExprHashMap<ExprHashSet> &lower);

private:
  struct Replacements {
    ExprHashMap<ref<Expr>> equalities;
    ExprHashMap<ref<Expr>> equalitiesParents;
  };

private:
  static Replacements gatherReplacements(constraints_ty constraints);
  static void addReplacement(Replacements &replacements, ref<Expr> expr);
  static void removeReplacement(Replacements &replacements, ref<Expr> expr);
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const ConstraintSet &constraints) {
  constraints.print(os);
  return os;
}

} // namespace klee

#endif /* KLEE_CONSTRAINTS_H */
