//===-- YicesBuilder.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Incorporating Yices into Klee was supported by DARPA under 
// contract N66001-13-C-4057.
//  
// Approved for Public Release, Distribution Unlimited.
//
// Bruno Dutertre & Ian A. Mason @  SRI International.
//===----------------------------------------------------------------------===//

#ifndef __UTIL_YICESBUILDER_H__
#define __UTIL_YICESBUILDER_H__



#include "klee/util/ExprHashMap.h"
#include "klee/util/ArrayExprHash.h"
#include "klee/Config/config.h"

#ifdef SUPPORT_YICES

#include "yices.h"
#define YicesExpr    term_t
#define YicesType    type_t
#define YicesConfig  ctx_config_t
#define YicesContext context_t
#define YicesModel model_t

#include <vector>

namespace klee {

  class YicesArrayExprHash : public ArrayExprHash< ::YicesExpr > {
    
    friend class YicesBuilder;
    
  public:
    YicesArrayExprHash() {};
    virtual ~YicesArrayExprHash();
  };


class YicesBuilder {
  ::YicesExpr tempVars[4];
  ExprHashMap< std::pair< ::YicesExpr, unsigned> > constructed;

  YicesArrayExprHash _arr_hash;

private:  

  ::YicesExpr bvOne(unsigned width);
  ::YicesExpr bvZero(unsigned width);
  ::YicesExpr bvMinusOne(unsigned width);
  ::YicesExpr bvConst32(unsigned width, uint32_t value);
  ::YicesExpr bvConst64(unsigned width, uint64_t value);
  ::YicesExpr bvZExtConst(unsigned width, uint64_t value);
  ::YicesExpr bvSExtConst(unsigned width, uint64_t value);

  ::YicesExpr bvBoolExtract(::YicesExpr expr, int bit);
  ::YicesExpr bvExtract(::YicesExpr expr, unsigned top, unsigned bottom);
  ::YicesExpr eqExpr(::YicesExpr a, ::YicesExpr b);

  //logical left and right shift (not arithmetic)
  ::YicesExpr bvLeftShift(::YicesExpr expr, unsigned shift);
  ::YicesExpr bvRightShift(::YicesExpr expr, unsigned shift);
  ::YicesExpr bvVarLeftShift(::YicesExpr expr, ::YicesExpr shift);
  ::YicesExpr bvVarRightShift(::YicesExpr expr, ::YicesExpr shift);
  ::YicesExpr bvVarArithRightShift(::YicesExpr expr, ::YicesExpr shift);

  ::YicesExpr constructAShrByConstant(::YicesExpr expr, unsigned shift, ::YicesExpr isSigned);
  ::YicesExpr constructMulByConstant(::YicesExpr expr, unsigned width, uint64_t x);
  ::YicesExpr constructUDivByConstant(::YicesExpr expr_n, unsigned width, uint64_t d);
  ::YicesExpr constructSDivByConstant(::YicesExpr expr_n, unsigned width, uint64_t d);

  ::YicesExpr getInitialArray(const Array *os);
  ::YicesExpr getArrayForUpdate(const Array *root, const UpdateNode *un);

  ::YicesExpr constructActual(ref<Expr> e, int *width_out);
  ::YicesExpr construct(ref<Expr> e, int *width_out);
  
  ::YicesExpr buildVar(const char *name, unsigned width);
  ::YicesExpr buildArray(const char *name, unsigned indexWidth, unsigned valueWidth);
 
public:
  YicesBuilder();
  ~YicesBuilder();

  ::YicesExpr getTrue();
  ::YicesExpr getFalse();
  ::YicesExpr getTempVar(Expr::Width w);
  ::YicesExpr getInitialRead(const Array *os, unsigned index);

  ::YicesExpr construct(ref<Expr> e) { 
    ::YicesExpr  res = construct(e, 0);
    constructed.clear();
    return res;
  }
};

}

#endif /* SUPPORT_YICES */

#endif /* __UTIL_YICESBUILDER_H__ */
