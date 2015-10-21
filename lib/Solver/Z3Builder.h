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

#include <z3++.h>

namespace klee {
  class Z3ExprHolder {
    friend class Z3ExprHandle;
    ::z3::expr expr;
    unsigned count;
    
  public:
    Z3ExprHolder(const ::z3::expr _expr) : expr(_expr), count(0) {}
    ~Z3ExprHolder() {
      if (expr) delete expr;
    }
  };

  class Z3ExprHandle {
    Z3ExprHolder *H;
    
  public:
    Z3ExprHandle() : H(new Z3ExprHolder(0)) { H->count++; }
    Z3ExprHandle(::z3::expr _expr) : H(new Z3ExprHolder(_expr)) { H->count++; }
    Z3ExprHandle(const Z3ExprHandle &b) : H(b.H) { H->count++; }
    ~Z3ExprHandle() { if (--H->count == 0) delete H; }
    
    Z3ExprHandle &operator=(const Z3ExprHandle &b) {
      if (--H->count == 0) delete H;
      H = b.H;
      H->count++;
      return *this;
    }

    operator bool () { return H->expr; }
    operator ::z3::expr () { return H->expr; }
  };
  
  class Z3ArrayExprHash : public ArrayExprHash< ::z3::expr > {
    
    friend class Z3Builder;
    
  public:
    Z3ArrayExprHash() {};
    virtual ~Z3ArrayExprHash();
  };

class Z3Builder {
  ::z3::context ctx;
  Z3ExprHandle tempVars[4];
  ExprHashMap< std::pair<Z3ExprHandle, unsigned> > constructed;

  /// optimizeDivides - Rewrite division and reminders by constants
  /// into multiplies and shifts. STP should probably handle this for
  /// use.
  bool optimizeDivides;

  Z3ArrayExprHash _arr_hash;

private:  

  Z3ExprHandle bvOne(unsigned width);
  Z3ExprHandle bvZero(unsigned width);
  Z3ExprHandle bvMinusOne(unsigned width);
  Z3ExprHandle bvConst32(unsigned width, uint32_t value);
  Z3ExprHandle bvConst64(unsigned width, uint64_t value);
  Z3ExprHandle bvZExtConst(unsigned width, uint64_t value);
  Z3ExprHandle bvSExtConst(unsigned width, uint64_t value);

  Z3ExprHandle bvBoolExtract(Z3ExprHandle expr, int bit);
  Z3ExprHandle bvExtract(Z3ExprHandle expr, unsigned top, unsigned bottom);
  Z3ExprHandle eqExpr(Z3ExprHandle a, Z3ExprHandle b);

  //logical left and right shift (not arithmetic)
  Z3ExprHandle bvLeftShift(Z3ExprHandle expr, unsigned shift);
  Z3ExprHandle bvRightShift(Z3ExprHandle expr, unsigned shift);
  Z3ExprHandle bvVarLeftShift(Z3ExprHandle expr, Z3ExprHandle shift);
  Z3ExprHandle bvVarRightShift(Z3ExprHandle expr, Z3ExprHandle shift);
  Z3ExprHandle bvVarArithRightShift(Z3ExprHandle expr, Z3ExprHandle shift);

  // Some STP-style bitvector arithmetic
  Z3ExprHandle bvMinusExpr(unsigned width, Z3ExprHandle minuend, Z3ExprHandle subtrahend);
  Z3ExprHandle bvPlusExpr(unsigned width, Z3ExprHandle augend, Z3ExprHandle addend);
  Z3ExprHandle bvMultExpr(unsigned width, Z3ExprHandle multiplacand, Z3ExprHandle multiplier);
  Z3ExprHandle bvDivExpr(unsigned width, Z3ExprHandle dividend, Z3ExprHandle divisor);
  Z3ExprHandle sbvDivExpr(unsigned width, Z3ExprHandle dividend, Z3ExprHandle divisor);
  Z3ExprHandle bvModExpr(unsigned width, Z3ExprHandle dividend, Z3ExprHandle divisor);
  Z3ExprHandle sbvModExpr(unsigned width, Z3ExprHandle dividend, Z3ExprHandle divisor);
  Z3ExprHandle notExpr(Z3ExprHandle expr);
  Z3ExprHandle bvNotExpr(Z3ExprHandle expr);
  Z3ExprHandle andExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle bvAndExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle orExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle bvOrExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle iffExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle eqExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle bvXorExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle bvSignExtend(Z3ExprHandle src, unsigned width);

  // Some STP-style array domain interface
  Z3ExprHandle writeExpr(Z3ExprHandle array, Z3ExprHandle index, Z3ExprHandle value);
  Z3ExprHandle readExpr(Z3ExprHandle array, Z3ExprHandle index);

  // ITE-expression constructor
  Z3ExprHandle iteExpr(Z3ExprHandle condition, Z3ExprHandle whenTrue, Z3ExprHandle whenFalse);

  // Bitvector length
  int getBVLength(Z3ExprHandle expr);

  // Bitvector comparison
  Z3ExprHandle bvLtExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle bvLeExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle sbvLtExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);
  Z3ExprHandle sbvLeExpr(Z3ExprHandle lhs, Z3ExprHandle rhs);

  Z3ExprHandle constructAShrByConstant(Z3ExprHandle expr, unsigned shift,
                                       Z3ExprHandle isSigned);
  Z3ExprHandle constructMulByConstant(Z3ExprHandle expr, unsigned width, uint64_t x);
  Z3ExprHandle constructUDivByConstant(Z3ExprHandle expr_n, unsigned width, uint64_t d);
  Z3ExprHandle constructSDivByConstant(Z3ExprHandle expr_n, unsigned width, uint64_t d);

  ::z3::expr getInitialArray(const Array *os);
  ::z3::expr getArrayForUpdate(const Array *root, const UpdateNode *un);

  Z3ExprHandle constructActual(ref<Expr> e, int *width_out);
  Z3ExprHandle construct(ref<Expr> e, int *width_out);
  
  ::z3::expr buildVar(const char *name, unsigned width);
  ::z3::expr buildArray(const char *name, unsigned indexWidth, unsigned valueWidth);
 
public:
  Z3Builder(::z3::context _ctx, bool _optimizeDivides=true);
  ~Z3Builder();

  Z3ExprHandle getTrue();
  Z3ExprHandle getFalse();
  Z3ExprHandle getTempVar(Expr::Width w);
  Z3ExprHandle getInitialRead(const Array *os, unsigned index);

  Z3ExprHandle construct(ref<Expr> e) {
    Z3ExprHandle res = construct(e, 0);
    constructed.clear();
    return res;
  }
};

}

#endif
