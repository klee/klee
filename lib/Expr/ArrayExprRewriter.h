//===-- ArrayExprRewriter.h -----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_ARRAYEXPRREWRITER_H
#define KLEE_ARRAYEXPRREWRITER_H

#include <iterator>
#include <map>
#include <vector>

#include "klee/Expr/Expr.h"
#include "klee/util/Ref.h"

namespace klee {

using array2idx_ty = std::map<const Array *, std::vector<ref<Expr>>>;
using mapIndexOptimizedExpr_ty = std::map<ref<Expr>, std::vector<ref<Expr>>>;

class ExprRewriter {
public:
  static ref<Expr> createOptExpr(const ref<Expr> &e, const array2idx_ty &arrays,
                                 const mapIndexOptimizedExpr_ty &idx_valIdx);

private:
  static ref<Expr> rewrite(const ref<Expr> &e, const array2idx_ty &arrays,
                           const mapIndexOptimizedExpr_ty &idx_valIdx);

  static ref<Expr>
  concatenateOrExpr(const std::vector<ref<Expr>>::const_iterator begin,
                    const std::vector<ref<Expr>>::const_iterator end);

  static ref<Expr> createEqExpr(const ref<Expr> &index,
                                const ref<Expr> &valIndex);

  static ref<Expr> createRangeExpr(const ref<Expr> &index,
                                   const ref<Expr> &valStart,
                                   const ref<Expr> &valEnd);
};
} // namespace klee

#endif /* KLEE_ARRAYEXPRREWRITER_H */
