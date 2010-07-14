//===-- STPBuilder.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __UTIL_STPBUILDER_H__
#define __UTIL_STPBUILDER_H__

#include "klee/util/ExprHashMap.h"
#include "klee/Config/config.h"

#include <vector>
#include <map>

#define Expr VCExpr
#ifdef HAVE_EXT_STP
#include <stp/c_interface.h>
#else
#include "../../stp/c_interface/c_interface.h"
#endif

#if ENABLE_STPLOG == 1
#include "stp/stplog.h"
#endif
#undef Expr

namespace klee {
  class ExprHolder {
    friend class ExprHandle;
    ::VCExpr expr;
    unsigned count;
    
  public:
    ExprHolder(const ::VCExpr _expr) : expr(_expr), count(0) {}
    ~ExprHolder() { 
      if (expr) vc_DeleteExpr(expr); 
    }
  };

  class ExprHandle {
    ExprHolder *H;
    
  public:
    ExprHandle() : H(new ExprHolder(0)) { H->count++; }
    ExprHandle(::VCExpr _expr) : H(new ExprHolder(_expr)) { H->count++; }
    ExprHandle(const ExprHandle &b) : H(b.H) { H->count++; }
    ~ExprHandle() { if (--H->count == 0) delete H; }
    
    ExprHandle &operator=(const ExprHandle &b) {
      if (--H->count == 0) delete H;
      H = b.H;
      H->count++;
      return *this;
    }

    operator bool () { return H->expr; }
    operator ::VCExpr () { return H->expr; }
  };

class STPBuilder {
  ::VC vc;
  ExprHandle tempVars[4];
  ExprHashMap< std::pair<ExprHandle, unsigned> > constructed;

  /// optimizeDivides - Rewrite division and reminders by constants
  /// into multiplies and shifts. STP should probably handle this for
  /// use.
  bool optimizeDivides;

private:
  unsigned getShiftBits(unsigned amount) {
    return (amount == 64) ? 6 : 5;
  }

  ExprHandle bvOne(unsigned width);
  ExprHandle bvZero(unsigned width);
  ExprHandle bvMinusOne(unsigned width);
  ExprHandle bvConst32(unsigned width, uint32_t value);
  ExprHandle bvConst64(unsigned width, uint64_t value);

  ExprHandle bvBoolExtract(ExprHandle expr, int bit);
  ExprHandle bvExtract(ExprHandle expr, unsigned top, unsigned bottom);
  ExprHandle eqExpr(ExprHandle a, ExprHandle b);

  //logical left and right shift (not arithmetic)
  ExprHandle bvLeftShift(ExprHandle expr, unsigned shift, unsigned shiftBits);
  ExprHandle bvRightShift(ExprHandle expr, unsigned amount, unsigned shiftBits);
  ExprHandle bvVarLeftShift(ExprHandle expr, ExprHandle amount, unsigned width);
  ExprHandle bvVarRightShift(ExprHandle expr, ExprHandle amount, unsigned width);
  ExprHandle bvVarArithRightShift(ExprHandle expr, ExprHandle amount, unsigned width);

  ExprHandle constructAShrByConstant(ExprHandle expr, unsigned shift, 
                                     ExprHandle isSigned, unsigned shiftBits);
  ExprHandle constructMulByConstant(ExprHandle expr, unsigned width, uint64_t x);
  ExprHandle constructUDivByConstant(ExprHandle expr_n, unsigned width, uint64_t d);
  ExprHandle constructSDivByConstant(ExprHandle expr_n, unsigned width, uint64_t d);

  ::VCExpr getInitialArray(const Array *os);
  ::VCExpr getArrayForUpdate(const Array *root, const UpdateNode *un);

  ExprHandle constructActual(ref<Expr> e, int *width_out);
  ExprHandle construct(ref<Expr> e, int *width_out);
  
  ::VCExpr buildVar(const char *name, unsigned width);
  ::VCExpr buildArray(const char *name, unsigned indexWidth, unsigned valueWidth);

public:
  STPBuilder(::VC _vc, bool _optimizeDivides=true);
  ~STPBuilder();

  ExprHandle getTrue();
  ExprHandle getFalse();
  ExprHandle getTempVar(Expr::Width w);
  ExprHandle getInitialRead(const Array *os, unsigned index);

  ExprHandle construct(ref<Expr> e) { 
    ExprHandle res = construct(e, 0);
    constructed.clear();
    return res;
  }
};

}

#endif
