#ifndef KLEE_INDEPENDENTCONSTRAINTSETUNION_H
#define KLEE_INDEPENDENTCONSTRAINTSETUNION_H

#include "klee/ADT/Either.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/IndependentSet.h"

namespace klee {
class IndependentConstraintSetUnion
    : public DisjointSetUnion<ref<ExprEitherSymcrete>, IndependentConstraintSet,
                              ExprEitherSymcreteHash, ExprEitherSymcreteCmp> {
public:
  Assignment concretization;

public:
  ExprHashMap<ref<Expr>> concretizedExprs;

public:
  void updateConcretization(const Assignment &c);
  void removeConcretization(const Assignment &remove);
  void reEvaluateConcretization(const Assignment &newConcretization);

  IndependentConstraintSetUnion getConcretizedVersion() const;
  IndependentConstraintSetUnion
  getConcretizedVersion(const Assignment &c) const;

  IndependentConstraintSetUnion();
  IndependentConstraintSetUnion(const constraints_ty &is,
                                const SymcreteOrderedSet &os,
                                const Assignment &c);
  IndependentConstraintSetUnion(ref<const IndependentConstraintSet> ics);

  void
  addIndependentConstraintSetUnion(const IndependentConstraintSetUnion &icsu);

  void getAllDependentConstraintSets(
      ref<Expr> e,
      std::vector<ref<const IndependentConstraintSet>> &result) const;
  void getAllIndependentConstraintSets(
      ref<Expr> e,
      std::vector<ref<const IndependentConstraintSet>> &result) const;

  void addExpr(ref<Expr> e);
  void addSymcrete(ref<Symcrete> s);
};
} // namespace klee

#endif /* KLEE_INDEPENDENTCONSTRAINTSETUNION_H */
