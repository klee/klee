#ifndef LIB_EXPRREWRITER_H_
#define LIB_EXPRREWRITER_H_

#include "klee/Expr.h"
#include "klee/CommandLine.h"
#include "klee/Constraints.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/util/ArrayExprVisitor.h"
#include "klee/util/Assignment.h"
#include "klee/util/Ref.h"
#include "klee/AssignmentGenerator.h"

namespace klee {

typedef std::vector<const Array *> v_arr_ty;
typedef std::map<const Array *, std::vector<ref<Expr> > > array2idx_ty;
typedef std::map<ref<Expr>, std::vector<ref<Expr> > > mapIndexOptimizedExpr_ty;

class ExprRewriter {
public:
  static ref<Expr> createOptExpr(
      const ref<Expr> &e, const array2idx_ty &arrays,
      const std::map<ref<Expr>, std::vector<ref<Expr> > > &idx_valIdx);

private:
  static ref<Expr>
  rewrite(const ref<Expr> &e, const array2idx_ty &arrays,
          const std::map<ref<Expr>, std::vector<ref<Expr> > > &idx_valIdx);

  static ref<Expr>
  concatenateOrExpr(const std::vector<ref<Expr> >::const_iterator begin,
                    const std::vector<ref<Expr> >::const_iterator end);

  static ref<Expr> createEqExpr(const ref<Expr> &index,
                                const ref<Expr> &valIndex);

  static ref<Expr> createRangeExpr(const ref<Expr> &index,
                                   const ref<Expr> &valStart,
                                   const ref<Expr> &valEnd);
};
}

#endif /* LIB_EXPRREWRITER_H_ */
