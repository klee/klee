//===-- Z3Builder.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __UTIL_Z3BUILDER_H__
#define __UTIL_Z3BUILDER_H__

#include "klee/util/ExprHashMap.h"
#include "klee/util/ArrayExprHash.h"
#include "klee/Config/config.h"

#include <vector>

#include <z3.h>

namespace klee {

  class Z3ArrayExprHash : public ArrayExprHash< Z3_ast > {
    
    friend class Z3Builder;
    
  public:
    Z3ArrayExprHash() {};
    virtual ~Z3ArrayExprHash();
  };

class Z3Builder {
  Z3_ast tempVars[4];
  ExprHashMap< std::pair<Z3_ast, unsigned> > constructed;

  /// optimizeDivides - Rewrite division and reminders by constants
  /// into multiplies and shifts. STP should probably handle this for
  /// use.
  bool optimizeDivides;

  Z3ArrayExprHash _arr_hash;

private:  

  Z3_ast bvOne(unsigned width);
  Z3_ast bvZero(unsigned width);
  Z3_ast bvMinusOne(unsigned width);
  Z3_ast bvConst32(unsigned width, uint32_t value);
  Z3_ast bvConst64(unsigned width, uint64_t value);
  Z3_ast bvZExtConst(unsigned width, uint64_t value);
  Z3_ast bvSExtConst(unsigned width, uint64_t value);

  Z3_ast bvBoolExtract(Z3_ast expr, int bit);
  Z3_ast bvExtract(Z3_ast expr, unsigned top, unsigned bottom);
  Z3_ast eqExpr(Z3_ast a, Z3_ast b);

  //logical left and right shift (not arithmetic)
  Z3_ast bvLeftShift(Z3_ast expr, unsigned shift);
  Z3_ast bvRightShift(Z3_ast expr, unsigned shift);
  Z3_ast bvVarLeftShift(Z3_ast expr, Z3_ast shift);
  Z3_ast bvVarRightShift(Z3_ast expr, Z3_ast shift);
  Z3_ast bvVarArithRightShift(Z3_ast expr, Z3_ast shift);

  // Some STP-style bitvector arithmetic
  Z3_ast bvMinusExpr(unsigned width, Z3_ast minuend, Z3_ast subtrahend);
  Z3_ast bvPlusExpr(unsigned width, Z3_ast augend, Z3_ast addend);
  Z3_ast bvMultExpr(unsigned width, Z3_ast multiplacand, Z3_ast multiplier);
  Z3_ast bvDivExpr(unsigned width, Z3_ast dividend, Z3_ast divisor);
  Z3_ast sbvDivExpr(unsigned width, Z3_ast dividend, Z3_ast divisor);
  Z3_ast bvModExpr(unsigned width, Z3_ast dividend, Z3_ast divisor);
  Z3_ast sbvModExpr(unsigned width, Z3_ast dividend, Z3_ast divisor);
  Z3_ast notExpr(Z3_ast expr);
  Z3_ast bvNotExpr(Z3_ast expr);
  Z3_ast andExpr(Z3_ast lhs, Z3_ast rhs);
  Z3_ast bvAndExpr(Z3_ast lhs, Z3_ast rhs);
  Z3_ast orExpr(Z3_ast lhs, Z3_ast rhs);
  Z3_ast bvOrExpr(Z3_ast lhs, Z3_ast rhs);
  Z3_ast iffExpr(Z3_ast lhs, Z3_ast rhs);
  Z3_ast bvXorExpr(Z3_ast lhs, Z3_ast rhs);
  Z3_ast bvSignExtend(Z3_ast src, unsigned width);

  // Some STP-style array domain interface
  Z3_ast writeExpr(Z3_ast array, Z3_ast index, Z3_ast value);
  Z3_ast readExpr(Z3_ast array, Z3_ast index);

  // ITE-expression constructor
  Z3_ast iteExpr(Z3_ast condition, Z3_ast whenTrue, Z3_ast whenFalse);

  // Bitvector length
  int getBVLength(Z3_ast expr);

  // Bitvector comparison
  Z3_ast bvLtExpr(Z3_ast lhs, Z3_ast rhs);
  Z3_ast bvLeExpr(Z3_ast lhs, Z3_ast rhs);
  Z3_ast sbvLtExpr(Z3_ast lhs, Z3_ast rhs);
  Z3_ast sbvLeExpr(Z3_ast lhs, Z3_ast rhs);

  Z3_ast constructAShrByConstant(Z3_ast expr, unsigned shift,
                                       Z3_ast isSigned);
  Z3_ast constructMulByConstant(Z3_ast expr, unsigned width, uint64_t x);
  Z3_ast constructUDivByConstant(Z3_ast expr_n, unsigned width, uint64_t d);
  Z3_ast constructSDivByConstant(Z3_ast expr_n, unsigned width, uint64_t d);

  Z3_ast getInitialArray(const Array *os);
  Z3_ast getArrayForUpdate(const Array *root, const UpdateNode *un);

  Z3_ast constructActual(ref<Expr> e, int *width_out);
  Z3_ast construct(ref<Expr> e, int *width_out);
  
  Z3_ast buildVar(const char *name, unsigned width);
  Z3_ast buildArray(const char *name, unsigned indexWidth, unsigned valueWidth);
 
public:
  Z3_context ctx;

  Z3Builder();
  ~Z3Builder();

  Z3_ast getTrue();
  Z3_ast getFalse();
  Z3_ast getTempVar(Expr::Width w);
  Z3_ast getInitialRead(const Array *os, unsigned index);

  Z3_ast construct(ref<Expr> e) {
    Z3_ast res = construct(e, 0);
    constructed.clear();
    return res;
  }

};

}

#endif
