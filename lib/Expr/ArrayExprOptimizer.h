//===-- ArrayExprOptimizer.h ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_ARRAYEXPROPTIMIZER_H
#define KLEE_ARRAYEXPROPTIMIZER_H

#include <cstdint>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "klee/Expr/Expr.h"
#include "klee/util/Ref.h"

namespace klee {

enum ArrayOptimizationType { NONE, ALL, INDEX, VALUE };

using array2idx_ty = std::map<const Array *, std::vector<ref<Expr>>>;
using mapIndexOptimizedExpr_ty = std::map<ref<Expr>, std::vector<ref<Expr>>>;

class ExprOptimizer {
private:
  std::unordered_map<unsigned, ref<Expr>> cacheExprOptimized;
  std::unordered_set<unsigned> cacheExprUnapplicable;
  std::unordered_map<unsigned, ref<Expr>> cacheReadExprOptimized;

public:
  /// Returns the optimised version of e.
  /// @param e expression to optimise
  /// @param valueOnly XXX document
  /// @return optimised expression
  ref<Expr> optimizeExpr(const ref<Expr> &e, bool valueOnly);

private:
  bool computeIndexes(array2idx_ty &arrays, const ref<Expr> &e,
                      mapIndexOptimizedExpr_ty &idx_valIdx) const;

  ref<Expr> getSelectOptExpr(
      const ref<Expr> &e, std::vector<const ReadExpr *> &reads,
      std::map<const ReadExpr *, std::pair<unsigned, Expr::Width>> &readInfo,
      bool isSymbolic);

  ref<Expr> buildConstantSelectExpr(const ref<Expr> &index,
                                    std::vector<uint64_t> &arrayValues,
                                    Expr::Width width,
                                    unsigned elementsInArray) const;

  ref<Expr>
  buildMixedSelectExpr(const ReadExpr *re,
                       std::vector<std::pair<uint64_t, bool>> &arrayValues,
                       Expr::Width width, unsigned elementsInArray) const;
};
} // namespace klee

#endif /* KLEE_ARRAYEXPROPTIMIZER_H */
