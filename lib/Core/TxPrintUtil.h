//===--- TxPrintUtil.h - Memory location dependency -------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Functions to help display object contents.
///
//===----------------------------------------------------------------------===//

#ifndef KLEE_TXPRINTUTIL_H
#define KLEE_TXPRINTUTIL_H

#include "klee/Config/Version.h"
#include "klee/Constraints.h"
#include "klee/Expr.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/Value.h>
#else
#include <llvm/Value.h>
#endif

#include <string>

namespace klee {

/// Encapsulates functionality of expression builder
class PrettyExpressionBuilder {

  std::string bvOne() {
    return "1";
  };
  std::string bvZero() {
    return "0";
  };
  std::string bvMinusOne() { return "-1"; }
  std::string bvConst32(uint32_t value);
  std::string bvConst64(uint64_t value);
  std::string bvZExtConst(uint64_t value);
  std::string bvSExtConst(uint64_t value);

  std::string bvBoolExtract(std::string expr, int bit);
  std::string bvExtract(std::string expr, unsigned top, unsigned bottom);
  std::string eqExpr(std::string a, std::string b);

  // logical left and right shift (not arithmetic)
  std::string bvLeftShift(std::string expr, unsigned shift);
  std::string bvRightShift(std::string expr, unsigned shift);
  std::string bvVarLeftShift(std::string expr, std::string shift);
  std::string bvVarRightShift(std::string expr, std::string shift);
  std::string bvVarArithRightShift(std::string expr, std::string shift);

  // Some STP-style bitvector arithmetic
  std::string bvMinusExpr(std::string minuend, std::string subtrahend);
  std::string bvPlusExpr(std::string augend, std::string addend);
  std::string bvMultExpr(std::string multiplacand, std::string multiplier);
  std::string bvDivExpr(std::string dividend, std::string divisor);
  std::string sbvDivExpr(std::string dividend, std::string divisor);
  std::string bvModExpr(std::string dividend, std::string divisor);
  std::string sbvModExpr(std::string dividend, std::string divisor);
  std::string notExpr(std::string expr);
  std::string bvAndExpr(std::string lhs, std::string rhs);
  std::string bvOrExpr(std::string lhs, std::string rhs);
  std::string iffExpr(std::string lhs, std::string rhs);
  std::string bvXorExpr(std::string lhs, std::string rhs);
  std::string bvSignExtend(std::string src);

  // Some STP-style array domain interface
  std::string writeExpr(std::string array, std::string index,
                        std::string value);
  std::string readExpr(std::string array, std::string index);

  // ITE-expression constructor
  std::string iteExpr(std::string condition, std::string whenTrue,
                      std::string whenFalse);

  // Bitvector comparison
  std::string bvLtExpr(std::string lhs, std::string rhs);
  std::string bvLeExpr(std::string lhs, std::string rhs);
  std::string sbvLtExpr(std::string lhs, std::string rhs);
  std::string sbvLeExpr(std::string lhs, std::string rhs);

  std::string existsExpr(std::string body);

  std::string constructAShrByConstant(std::string expr, unsigned shift,
                                      std::string isSigned);
  std::string constructMulByConstant(std::string expr, uint64_t x);
  std::string constructUDivByConstant(std::string expr_n, uint64_t d);
  std::string constructSDivByConstant(std::string expr_n, uint64_t d);

  std::string getInitialArray(const Array *root);
  std::string getArrayForUpdate(const Array *root, const UpdateNode *un);

  std::string constructActual(ref<Expr> e);

  std::string buildArray(const char *name, unsigned indexWidth,
                         unsigned valueWidth);

  std::string getTrue();
  std::string getFalse();
  std::string getInitialRead(const Array *root, unsigned index);

  PrettyExpressionBuilder();

  ~PrettyExpressionBuilder();

public:
  static std::string construct(ref<Expr> e);

  static std::string constructQuery(ConstraintManager &constraints,
                                    ref<Expr> e);
};

/// \brief Output function name to the output stream
extern bool outputFunctionName(llvm::Value *value, llvm::raw_ostream &stream);

/// \brief Create 8 times padding amount of spaces
extern std::string makeTabs(const unsigned paddingAmount);

/// \brief Create 1 padding amount of spaces
extern std::string appendTab(const std::string &prefix);
}

#endif
