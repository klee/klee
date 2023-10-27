//===-- AlphaBuilder.h -----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_ALPHA_BUILDER_H
#define KLEE_ALPHA_BUILDER_H

#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprVisitor.h"

namespace klee {

class AlphaBuilder final : public ExprVisitor {
public:
  ExprHashMap<ref<Expr>> reverseExprMap;
  ArrayCache::ArrayHashMap<const Array *> reverseAlphaArrayMap;
  ArrayCache::ArrayHashMap<const Array *> alphaArrayMap;

private:
  ArrayCache &arrayCache;
  unsigned index = 0;

  const Array *visitArray(const Array *arr);
  UpdateList visitUpdateList(UpdateList u);
  Action visitRead(const ReadExpr &re) override;
  using ExprVisitor::visitExpr;

public:
  AlphaBuilder(ArrayCache &_arrayCache);
  constraints_ty visitConstraints(constraints_ty cs);
  ref<Expr> build(ref<Expr> v);
  const Array *buildArray(const Array *arr) { return visitArray(arr); }
};

} // namespace klee

#endif /*KLEE_ALPHA_VERSION_BUILDER_H*/
