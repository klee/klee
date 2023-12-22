//===-- Z3BitvectorBuilder.h -----------------------------------*- C++ -*-====//
//-*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_Z3_BITVECTOR_BUILDER_H
#define KLEE_Z3_BITVECTOR_BUILDER_H
#include "Z3BitvectorBuilder.h"
#include "Z3Builder.h"
#include "klee/Config/config.h"
#include "klee/Expr/ArrayExprHash.h"
#include "klee/Expr/ExprHashMap.h"

#include <unordered_map>
#include <z3.h>

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/APFloat.h"
DISABLE_WARNING_POP

namespace klee {
class Z3BitvectorBuilder : public Z3Builder {
private:
  void FPCastWidthAssert(int *width_out, char const *msg);

protected:
  Z3ASTHandle bvExtract(Z3ASTHandle expr, unsigned top,
                        unsigned bottom) override;
  Z3ASTHandle eqExpr(Z3ASTHandle a, Z3ASTHandle b) override;

  // logical left and right shift (not arithmetic)
  Z3ASTHandle bvLeftShift(Z3ASTHandle expr, unsigned shift) override;
  Z3ASTHandle bvRightShift(Z3ASTHandle expr, unsigned shift) override;
  Z3ASTHandle bvVarLeftShift(Z3ASTHandle expr, Z3ASTHandle shift) override;
  Z3ASTHandle bvVarRightShift(Z3ASTHandle expr, Z3ASTHandle shift) override;
  Z3ASTHandle bvVarArithRightShift(Z3ASTHandle expr,
                                   Z3ASTHandle shift) override;

  Z3ASTHandle bvNotExpr(Z3ASTHandle expr) override;
  Z3ASTHandle bvAndExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) override;
  Z3ASTHandle bvOrExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) override;
  Z3ASTHandle bvXorExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) override;
  Z3ASTHandle bvSignExtend(Z3ASTHandle src, unsigned width) override;

  // ITE-expression constructor
  Z3ASTHandle iteExpr(Z3ASTHandle condition, Z3ASTHandle whenTrue,
                      Z3ASTHandle whenFalse) override;

  // Bitvector comparison
  Z3ASTHandle bvLtExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) override;
  Z3ASTHandle bvLeExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) override;
  Z3ASTHandle sbvLtExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) override;
  Z3ASTHandle sbvLeExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) override;

  Z3ASTHandle constructAShrByConstant(Z3ASTHandle expr, unsigned shift,
                                      Z3ASTHandle isSigned) override;

  Z3ASTHandle constructActual(ref<Expr> e, int *width_out) override;

  Z3SortHandle getFloatSortFromBitWidth(unsigned bitWidth);

  // Float casts
  Z3ASTHandle castToFloat(const Z3ASTHandle &e);
  Z3ASTHandle castToBitVector(const Z3ASTHandle &e);

  Z3ASTHandle getRoundingModeSort(llvm::APFloat::roundingMode rm);
  Z3ASTHandle getx87FP80ExplicitSignificandIntegerBit(const Z3ASTHandle &e);

public:
  Z3BitvectorBuilder(bool autoClearConstructCache,
                     const char *z3LogInteractionFile);
  ~Z3BitvectorBuilder();
};
} // namespace klee
#endif
