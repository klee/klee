//===-- YicesBuilder.cpp ----------------------------------------------------===//
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


#include "YicesBuilder.h"

#include "klee/Expr.h"
#include "klee/Solver.h"
#include "klee/util/Bits.h"

#include "ConstantDivision.h"
#include "SolverStats.h"

#include "llvm/Support/CommandLine.h"

#ifdef SUPPORT_YICES

#include <cstdio>

#include <algorithm> // max, min
#include <cassert>
#include <map>
#include <sstream>
#include <iostream>
#include <vector>

using namespace klee;

namespace {
  llvm::cl::opt<bool>
  UseConstructHash("use-construct-hash-yices", 
                   llvm::cl::desc("Use hash-consing during Yices query construction."),
                   llvm::cl::init(true));
}


YicesArrayExprHash::~YicesArrayExprHash() {
  
}


YicesBuilder::YicesBuilder() 
{
  yices_init();

  tempVars[0] = buildVar("__tmpInt8", 8);
  tempVars[1] = buildVar("__tmpInt16", 16);
  tempVars[2] = buildVar("__tmpInt32", 32);
  tempVars[3] = buildVar("__tmpInt64", 64);
}

YicesBuilder::~YicesBuilder() {
  yices_exit();
}

::YicesExpr YicesBuilder::buildVar(const char *name, unsigned width) {
  ::YicesType t = (width==1) ? yices_bool_type() : yices_bv_type(width);
  ::YicesExpr res = yices_new_uninterpreted_term(t);
  yices_set_term_name(res, name);
  return res;
}

::YicesExpr YicesBuilder::buildArray(const char *name, unsigned indexWidth, unsigned valueWidth) {
  ::YicesType t1 = yices_bv_type(indexWidth);
  ::YicesType t2 = yices_bv_type(valueWidth);
  ::YicesType t = yices_function_type1(t1, t2);
  ::YicesExpr res =  yices_new_uninterpreted_term(t);
  yices_set_term_name(res, name);
  return res;
}

::YicesExpr YicesBuilder::getTempVar(Expr::Width w) {
  switch (w) {
  default: assert(0 && "invalid type");
  case Expr::Int8:  return tempVars[0];
  case Expr::Int16: return tempVars[1];
  case Expr::Int32: return tempVars[2];
  case Expr::Int64: return tempVars[3];
  }
}

::YicesExpr YicesBuilder::getTrue() {
  return yices_true();
}
::YicesExpr YicesBuilder::getFalse() {
  return yices_false();
}
::YicesExpr YicesBuilder::bvOne(unsigned width) {
  return yices_bvconst_one(width);
}
::YicesExpr YicesBuilder::bvZero(unsigned width) {
  return yices_bvconst_zero(width);
}
::YicesExpr YicesBuilder::bvMinusOne(unsigned width) {
  return yices_bvconst_minus_one(width);
}
::YicesExpr YicesBuilder::bvConst32(unsigned width, uint32_t value) {
  return yices_bvconst_uint32(width, value);
}
::YicesExpr YicesBuilder::bvConst64(unsigned width, uint64_t value) {
  return yices_bvconst_uint64(width, value);
}
::YicesExpr YicesBuilder::bvZExtConst(unsigned width, uint64_t value) {
  return yices_bvconst_uint64(width, value);
}
::YicesExpr YicesBuilder::bvSExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return yices_bvconst_uint64(width, value);
  return yices_sign_extend(yices_bvconst_uint64(64, value), width);
}

::YicesExpr YicesBuilder::bvBoolExtract(::YicesExpr expr, int bit) {
  return yices_bitextract(expr, bit);
}
::YicesExpr YicesBuilder::bvExtract(::YicesExpr expr, unsigned top, unsigned bottom) {
  assert(bottom <= top);
  return yices_bvextract(expr, bottom, top);
}
::YicesExpr YicesBuilder::eqExpr(::YicesExpr a, ::YicesExpr b) {
  return yices_eq(a, b);
}

// logical right shift
::YicesExpr YicesBuilder::bvRightShift(::YicesExpr expr, unsigned shift) {
  uint32_t width = yices_term_bitsize(expr);
  if (shift > width)
    return yices_bvconst_zero(width);
  else 
    return yices_shift_right0(expr, shift);
}

// logical left shift
::YicesExpr YicesBuilder::bvLeftShift(::YicesExpr expr, unsigned shift) {
  uint32_t width = yices_term_bitsize(expr);
  if (shift > width)
    return yices_bvconst_zero(width);
  else
    return yices_shift_left0(expr, shift);
}

// left shift by a variable amount on an expression of the specified width
::YicesExpr YicesBuilder::bvVarLeftShift(::YicesExpr expr, ::YicesExpr shift) {
  return yices_bvshl(expr, shift);
}

// logical right shift by a variable amount on an expression of the specified width
::YicesExpr YicesBuilder::bvVarRightShift(::YicesExpr expr, ::YicesExpr shift) {
  return yices_bvlshr(expr, shift);
}

// arithmetic right shift by a variable amount on an expression of the specified width
::YicesExpr YicesBuilder::bvVarArithRightShift(::YicesExpr expr, ::YicesExpr shift) {
  return yices_bvashr(expr, shift);
}

::YicesExpr YicesBuilder::constructAShrByConstant(::YicesExpr expr, unsigned shift, ::YicesExpr isSigned) {
  uint32_t width = yices_term_bitsize(expr);
  if (shift >= width)
    return yices_bvconst_zero(width);
  else
    return yices_ite(isSigned,
                     yices_shift_right1(expr, shift),
                     yices_shift_right0(expr, shift));
}

::YicesExpr YicesBuilder::constructMulByConstant(::YicesExpr expr, unsigned width, uint64_t x) {
  return yices_bvmul(expr, yices_bvconst_uint64(width, x));
}

/* 
 * Compute the 32-bit unsigned integer division of n by a divisor d based on 
 * the constants derived from the constant divisor d.
 *
 * Returns n/d without doing explicit division.  The cost is 2 adds, 3 shifts, 
 * and a (64-bit) multiply.
 *
 * @param n      numerator (dividend) as an expression
 * @param width  number of bits used to represent the value
 * @param d      the divisor
 *
 * @return n/d without doing explicit division
 */
::YicesExpr YicesBuilder::constructUDivByConstant(::YicesExpr expr_n, unsigned width, uint64_t d) {
  assert(width==32 && "can only compute udiv constants for 32-bit division");
  return yices_bvdiv(expr_n, yices_bvconst_uint64(width, d));
}

/* 
 * Compute the 32-bitnsigned integer division of n by a divisor d based on 
 * the constants derived from the constant divisor d.
 *
 * Returns n/d without doing explicit division.  The cost is 3 adds, 3 shifts, 
 * a (64-bit) multiply, and an XOR.
 *
 * @param n      numerator (dividend) as an expression
 * @param width  number of bits used to represent the value
 * @param d      the divisor
 *
 * @return n/d without doing explicit division
 */
::YicesExpr YicesBuilder::constructSDivByConstant(::YicesExpr expr_n, unsigned width, uint64_t d) {
  assert(width==32 && "can only compute udiv constants for 32-bit division");
  return yices_bvsdiv(expr_n, yices_bvconst_uint64(width, d));
}

::YicesExpr YicesBuilder::getInitialArray(const Array *root) {
  assert(root);
  ::YicesExpr array_expr;
  bool hashed = _arr_hash.lookupArrayExpr(root, array_expr);
  if (!hashed) {
    char buf[32];
    unsigned const addrlen = sprintf(buf, "_%p", (const void*)root) + 1;  // +1 for null-termination
    unsigned const space = (root->name.length() > 32 - addrlen)?(32 - addrlen):root->name.length();
    memmove(buf + space, buf, addrlen); // moving the address part to the end
    memcpy(buf, root->name.c_str(), space); // filling out the name part

    array_expr = buildArray(buf, root->getDomain(), root->getRange());

    if (root->isConstantArray()) {
      for (unsigned i = 0, e = root->size; i != e; ++i) {
        ::YicesExpr aux = construct(ConstantExpr::alloc(i, root->getDomain()), 0);
        array_expr = yices_update1(array_expr, aux, construct(root->constantValues[i], 0));
      }
    }
    
    _arr_hash.hashArrayExpr(root, array_expr);

  }
  return array_expr;
}

::YicesExpr YicesBuilder::getInitialRead(const Array *root, unsigned index) {
  ::YicesExpr aux = bvConst32(32, index);
  return yices_application1(getInitialArray(root), aux);
}

::YicesExpr YicesBuilder::getArrayForUpdate(const Array *root, const UpdateNode *un) {
  if (!un) {
    return getInitialArray(root);
  }
  else {
    // FIXME: This really needs to be non-recursive.
    ::YicesExpr un_expr;
    bool hashed = _arr_hash.lookupUpdateNodeExpr(un, un_expr);
    
    if (!hashed) {
      ::YicesExpr aux = construct(un->index, 0);
      un_expr = yices_update1(getArrayForUpdate(root, un->next), aux, construct(un->value, 0));
      _arr_hash.hashUpdateNodeExpr(un, un_expr);
    }
    
    return un_expr;
  }
}
  
  /** if *width_out!=1 then result is a bitvector, otherwise it is a bool */
::YicesExpr YicesBuilder::construct(ref<Expr> e, int *width_out) {
  ::YicesExpr retval = NULL_TERM;
  if (!UseConstructHash || isa<ConstantExpr>(e)) {
    retval = constructActual(e, width_out);
  } else {
    ExprHashMap< std::pair< ::YicesExpr, unsigned> >::iterator it = 
      constructed.find(e);
    if (it!=constructed.end()) {
      if (width_out)
        *width_out = it->second.second;
      retval = it->second.first;
    } else {
      int width;
      if (!width_out) width_out = &width;
      ::YicesExpr res = constructActual(e, width_out);
      constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
      retval = res;
    }
  }
  if(retval < 0){
    yices_print_error(stderr);
    assert(retval >= 0);
  }
  return retval;
}


/** if *width_out!=1 then result is a bitvector, otherwise it is a bool */
::YicesExpr YicesBuilder::constructActual(ref<Expr> e, int *width_out) {
  int width;
  if (!width_out) width_out = &width;

  ++stats::queryConstructs;

  switch (e->getKind()) {
  case Expr::Constant: {
    ConstantExpr *CE = cast<ConstantExpr>(e);
    *width_out = CE->getWidth();

    // Coerce to bool if necessary.
    if (*width_out == 1)
      return CE->isTrue() ? getTrue() : getFalse();

    // Fast path.
    if (*width_out <= 32)
      return bvConst32(*width_out, CE->getZExtValue(32));
    if (*width_out <= 64)
      return bvConst64(*width_out, CE->getZExtValue());

    unsigned width = CE->getWidth();
    int32_t* bits = new int32_t[width]; 

    for(unsigned i = 0; i < width; i++){
      bits[i] = CE->Extract(i, 1)->getZExtValue();
    }

    ::YicesExpr retval = yices_bvconst_from_array(width, bits);

    delete bits;

    return  retval;

  }
    
  // Special
  case Expr::NotOptimized: {
    NotOptimizedExpr *noe = cast<NotOptimizedExpr>(e);
    return construct(noe->src, width_out);
  }

  case Expr::Read: {
    ReadExpr *re = cast<ReadExpr>(e);
    assert(re && re->updates.root);
    *width_out = re->updates.root->getRange();
    ::YicesExpr aux = construct(re->index, 0);
    return  yices_application1(getArrayForUpdate(re->updates.root, re->updates.head), aux);
  }
    
  case Expr::Select: {
    SelectExpr *se = cast<SelectExpr>(e);
    ::YicesExpr cond = construct(se->cond, 0);
    ::YicesExpr tExpr = construct(se->trueExpr, width_out);
    ::YicesExpr fExpr = construct(se->falseExpr, width_out);
    return yices_ite(cond, tExpr, fExpr);
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    unsigned numKids = ce->getNumKids();
    ::YicesExpr res = construct(ce->getKid(numKids-1), 0);
    for (int i=numKids-2; i>=0; i--) {
      res = yices_bvconcat2(construct(ce->getKid(i), 0), res);
    }
    *width_out = ce->getWidth();
    return res;
  }

  case Expr::Extract: {
    ExtractExpr *ee = cast<ExtractExpr>(e);
    ::YicesExpr src = construct(ee->expr, width_out);    
    *width_out = ee->getWidth();
    if (*width_out==1) {
      return bvBoolExtract(src, ee->offset);
    } else {
      return yices_bvextract(src, ee->offset, ee->offset + *width_out - 1);
    }
  }

    // Casting
    
  case Expr::ZExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    ::YicesExpr src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth==1) {
      return yices_ite(src, bvOne(*width_out), bvZero(*width_out));
    } else {
      return yices_zero_extend(src, *width_out-srcWidth);
    }
  }

  case Expr::SExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    ::YicesExpr src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth==1) {
      return yices_ite(src, bvOne(*width_out), bvZero(*width_out));
    } else {
      return yices_sign_extend(src, *width_out-srcWidth);
    }
  }

    // Arithmetic

  case Expr::Add: {
    AddExpr *ae = cast<AddExpr>(e);
    ::YicesExpr left = construct(ae->left, width_out);
    ::YicesExpr right = construct(ae->right, width_out);
    assert(*width_out!=1 && "uncanonicalized add");
    return yices_bvadd(left, right);
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    ::YicesExpr left = construct(se->left, width_out);
    ::YicesExpr right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized sub");
    return yices_bvsub(left, right);
  } 

  case Expr::Mul: {
    MulExpr *se = cast<MulExpr>(e);
    ::YicesExpr left = construct(se->left, width_out);
    ::YicesExpr right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized mul");
    return yices_bvmul(left, right);
  }

  case Expr::UDiv: {
    UDivExpr *se = cast<UDivExpr>(e);
    ::YicesExpr left = construct(se->left, width_out);
    ::YicesExpr right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized udiv");
    return yices_bvdiv(left, right);
  }

  case Expr::SDiv: {
    SDivExpr *se = cast<SDivExpr>(e);
    ::YicesExpr left = construct(se->left, width_out);
    ::YicesExpr right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized sdiv");
    return yices_bvsdiv(left, right);
  }

  case Expr::URem: {
    URemExpr *se = cast<URemExpr>(e);
    ::YicesExpr left = construct(se->left, width_out);
    ::YicesExpr right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized urem");
    return yices_bvrem(left, right);
  }

  case Expr::SRem: {
    SRemExpr *se = cast<SRemExpr>(e);
    ::YicesExpr left = construct(se->left, width_out);
    ::YicesExpr right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized srem");
    return yices_bvsrem(left, right);
  }

    // Bitwise

  case Expr::Not: {
    NotExpr *ne = cast<NotExpr>(e);
    ::YicesExpr expr = construct(ne->expr, width_out);
    if (*width_out==1) {
      return yices_not(expr);
    } else {
      return yices_bvnot(expr);
    }
  }    

  case Expr::And: {
    AndExpr *ae = cast<AndExpr>(e);
    ::YicesExpr left = construct(ae->left, width_out);
    ::YicesExpr right = construct(ae->right, width_out);
    if (*width_out==1) {
      return yices_and2(left, right);
    } else {
      return yices_bvand2(left, right);
    }
  }

  case Expr::Or: {
    OrExpr *ae = cast<OrExpr>(e);
    ::YicesExpr left = construct(ae->left, width_out);
    ::YicesExpr right = construct(ae->right, width_out);
    if (*width_out==1) {
      return yices_or2(left, right);
    } else {
      return yices_bvor2(left, right);
    }
  }

  case Expr::Xor: {
    XorExpr *ae = cast<XorExpr>(e);
    ::YicesExpr left = construct(ae->left, width_out);
    ::YicesExpr right = construct(ae->right, width_out);
    if (*width_out==1) {
      return yices_xor2(left, right);
    } else {
      return yices_bvxor2(left, right);
    }
  }

  case Expr::Shl: {
    ShlExpr *se = cast<ShlExpr>(e);
    ::YicesExpr left = construct(se->left, width_out);
    assert(*width_out!=1 && "uncanonicalized shl");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
      return bvLeftShift(left, (unsigned) CE->getLimitedValue());
    } else {
      int shiftWidth;
      ::YicesExpr amount = construct(se->right, &shiftWidth);
      return bvVarLeftShift( left, amount);
    }
  }

  case Expr::LShr: {
    LShrExpr *lse = cast<LShrExpr>(e);
    ::YicesExpr left = construct(lse->left, width_out);
    assert(*width_out!=1 && "uncanonicalized lshr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
      return bvRightShift(left, (unsigned) CE->getLimitedValue());
    } else {
      int shiftWidth;
      ::YicesExpr amount = construct(lse->right, &shiftWidth);
      return bvVarRightShift( left, amount);
    }
  }

  case Expr::AShr: {
    AShrExpr *ase = cast<AShrExpr>(e);
    ::YicesExpr left = construct(ase->left, width_out);
    assert(*width_out!=1 && "uncanonicalized ashr");
    
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ase->right)) {
      unsigned shift = (unsigned) CE->getLimitedValue();
      ::YicesExpr signedBool = bvBoolExtract(left, *width_out-1);
      return constructAShrByConstant(left, shift, signedBool);
    } else {
      int shiftWidth;
      ::YicesExpr amount = construct(ase->right, &shiftWidth);
      return bvVarArithRightShift( left, amount);
    }
  }

    // Comparison

  case Expr::Eq: {
    EqExpr *ae = cast<EqExpr>(e);
    ::YicesExpr left = construct(ae->left, width_out);
    ::YicesExpr right = construct(ae->right, width_out);
    if (*width_out==1) {
      return yices_iff(left, right);
    } else {
      *width_out = 1;
      return yices_bveq_atom(left, right);
    }
  }

  case Expr::Ult: {
    UltExpr *ae = cast<UltExpr>(e);
    ::YicesExpr left = construct(ae->left, width_out);
    ::YicesExpr right = construct(ae->right, width_out);
    *width_out = 1;
    return yices_bvlt_atom(left, right);
  }

  case Expr::Ule: {
    UleExpr *ae = cast<UleExpr>(e);
    ::YicesExpr left = construct(ae->left, width_out);
    ::YicesExpr right = construct(ae->right, width_out);
    *width_out = 1;
    return yices_bvle_atom(left, right);
  }

  case Expr::Slt: {
    SltExpr *ae = cast<SltExpr>(e);
    ::YicesExpr left = construct(ae->left, width_out);
    ::YicesExpr right = construct(ae->right, width_out);
    *width_out = 1;
    return yices_bvslt_atom(left, right);
  }

  case Expr::Sle: {
    SleExpr *ae = cast<SleExpr>(e);
    ::YicesExpr left = construct(ae->left, width_out);
    ::YicesExpr right = construct(ae->right, width_out);
    *width_out = 1;
    return yices_bvsle_atom(left, right);
  }

    // unused due to canonicalization
#if 0
  case Expr::Ne:
  case Expr::Ugt:
  case Expr::Uge:
  case Expr::Sgt:
  case Expr::Sge:
#endif

  default: 
    assert(0 && "unhandled Expr type");
    return 0;
  }
}


#endif /* SUPPORT_YICES */

