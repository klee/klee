//===-- STPBuilder.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"
#ifdef ENABLE_STP
#include "STPBuilder.h"

#include "klee/Expr.h"
#include "klee/Solver.h"
#include "klee/util/Bits.h"
#include "klee/SolverStats.h"

#include "ConstantDivision.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"

#include <cstdio>

#define vc_bvBoolExtract IAMTHESPAWNOFSATAN
// unclear return
#define vc_bvLeftShiftExpr IAMTHESPAWNOFSATAN
#define vc_bvRightShiftExpr IAMTHESPAWNOFSATAN
// bad refcnt'ng
#define vc_bvVar32LeftShiftExpr IAMTHESPAWNOFSATAN
#define vc_bvVar32RightShiftExpr IAMTHESPAWNOFSATAN
#define vc_bvVar32DivByPowOfTwoExpr IAMTHESPAWNOFSATAN
#define vc_bvCreateMemoryArray IAMTHESPAWNOFSATAN
#define vc_bvReadMemoryArray IAMTHESPAWNOFSATAN
#define vc_bvWriteToMemoryArray IAMTHESPAWNOFSATAN

#include <algorithm> // max, min
#include <cassert>
#include <map>
#include <sstream>
#include <vector>

using namespace klee;

namespace {
  llvm::cl::opt<bool>
  UseConstructHash("use-construct-hash", 
                   llvm::cl::desc("Use hash-consing during STP query construction."),
                   llvm::cl::init(true));
}

///


STPArrayExprHash::~STPArrayExprHash() {
  for (ArrayHashIter it = _array_hash.begin(); it != _array_hash.end(); ++it) {
    ::VCExpr array_expr = it->second;
    if (array_expr) {
      ::vc_DeleteExpr(array_expr);
      array_expr = 0;
    }
  }


  for (UpdateNodeHashConstIter it = _update_node_hash.begin();
      it != _update_node_hash.end(); ++it) {
    ::VCExpr un_expr = it->second;
    if (un_expr) {
      ::vc_DeleteExpr(un_expr);
      un_expr = 0;
    }
  }
}

/***/

STPBuilder::STPBuilder(::VC _vc, bool _optimizeDivides)
  : vc(_vc), optimizeDivides(_optimizeDivides) {

}

STPBuilder::~STPBuilder() {
  
}

///

/* Warning: be careful about what c_interface functions you use. Some of
   them look like they cons memory but in fact don't, which is bad when
   you call vc_DeleteExpr on them. */

::VCExpr STPBuilder::buildVar(const char *name, unsigned width) {
  // XXX don't rebuild if this stuff cons's
  ::Type t = (width==1) ? vc_boolType(vc) : vc_bvType(vc, width);
  ::VCExpr res = vc_varExpr(vc, const_cast<char*>(name), t);
  vc_DeleteExpr(t);
  return res;
}

::VCExpr STPBuilder::buildArray(const char *name, unsigned indexWidth, unsigned valueWidth) {
  // XXX don't rebuild if this stuff cons's
  ::Type t1 = vc_bvType(vc, indexWidth);
  ::Type t2 = vc_bvType(vc, valueWidth);
  ::Type t = vc_arrayType(vc, t1, t2);
  ::VCExpr res = vc_varExpr(vc, const_cast<char*>(name), t);
  vc_DeleteExpr(t);
  vc_DeleteExpr(t2);
  vc_DeleteExpr(t1);
  return res;
}

ExprHandle STPBuilder::getTrue() {
  return vc_trueExpr(vc);
}
ExprHandle STPBuilder::getFalse() {
  return vc_falseExpr(vc);
}
ExprHandle STPBuilder::bvOne(unsigned width) {
  return bvZExtConst(width, 1);
}
ExprHandle STPBuilder::bvZero(unsigned width) {
  return bvZExtConst(width, 0);
}
ExprHandle STPBuilder::bvMinusOne(unsigned width) {
  return bvSExtConst(width, (int64_t) -1);
}
ExprHandle STPBuilder::bvConst32(unsigned width, uint32_t value) {
  return vc_bvConstExprFromInt(vc, width, value);
}
ExprHandle STPBuilder::bvConst64(unsigned width, uint64_t value) {
  return vc_bvConstExprFromLL(vc, width, value);
}
ExprHandle STPBuilder::bvZExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  ExprHandle expr = bvConst64(64, value), zero = bvConst64(64, 0);
  for (width -= 64; width > 64; width -= 64)
    expr = vc_bvConcatExpr(vc, zero, expr);
  return vc_bvConcatExpr(vc, bvConst64(width, 0), expr);
}
ExprHandle STPBuilder::bvSExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);
  return vc_bvSignExtend(vc, bvConst64(64, value), width);
}

ExprHandle STPBuilder::bvBoolExtract(ExprHandle expr, int bit) {
  return vc_eqExpr(vc, bvExtract(expr, bit, bit), bvOne(1));
}
ExprHandle STPBuilder::bvExtract(ExprHandle expr, unsigned top, unsigned bottom) {
  return vc_bvExtract(vc, expr, top, bottom);
}
ExprHandle STPBuilder::eqExpr(ExprHandle a, ExprHandle b) {
  assert((vc_getBVLength(vc, a) == vc_getBVLength(vc, b)) && "a and b should be same type");
  return vc_eqExpr(vc, a, b);
}

// logical right shift
ExprHandle STPBuilder::bvRightShift(ExprHandle expr, unsigned shift) {
  unsigned width = vc_getBVLength(vc, expr);

  if (shift==0) {
    return expr;
  } else if (shift>=width) {
    return bvZero(width); // Overshift to zero
  } else {
    return vc_bvConcatExpr(vc,
                           bvZero(shift),
                           bvExtract(expr, width - 1, shift));
  }
}

// logical left shift
ExprHandle STPBuilder::bvLeftShift(ExprHandle expr, unsigned shift) {
  unsigned width = vc_getBVLength(vc, expr);

  if (shift==0) {
    return expr;
  } else if (shift>=width) {
    return bvZero(width); // Overshift to zero
  } else {
    // stp shift does "expr @ [0 x s]" which we then have to extract,
    // rolling our own gives slightly smaller exprs
    return vc_bvConcatExpr(vc, 
                           bvExtract(expr, width - shift - 1, 0),
                           bvZero(shift));
  }
}

ExprHandle STPBuilder::extractPartialShiftValue(ExprHandle shift,
                                                unsigned width,
                                                unsigned &shiftBits) {
  // Assuming width is power of 2
  llvm::APInt sw(32, width);
  shiftBits = sw.getActiveBits();

  // get the shift amount (looking only at the bits appropriate for the given
  // width)
  return vc_bvExtract(vc, shift, shiftBits - 1, 0);
}

// left shift by a variable amount on an expression of the specified width
ExprHandle STPBuilder::bvVarLeftShift(ExprHandle expr, ExprHandle shift) {
  unsigned width = vc_getBVLength(vc, expr);
  ExprHandle res = bvZero(width);

  unsigned shiftBits = 0;
  ExprHandle shift_ext = extractPartialShiftValue(shift, width, shiftBits);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  for (int i = width - 1; i >= 0; i--) {
    res = vc_iteExpr(vc, eqExpr(shift_ext, bvConst32(shiftBits, i)),
                     bvLeftShift(expr, i), res);
  }

  // If overshifting, shift to zero
  ExprHandle ex =
      vc_bvLtExpr(vc, shift, bvConst32(vc_getBVLength(vc, shift), width));

  res = vc_iteExpr(vc, ex, res, bvZero(width));
  return res;
}

// logical right shift by a variable amount on an expression of the specified
// width
ExprHandle STPBuilder::bvVarRightShift(ExprHandle expr, ExprHandle shift) {
  unsigned width = vc_getBVLength(vc, expr);
  ExprHandle res = bvZero(width);

  unsigned shiftBits = 0;
  ExprHandle shift_ext = extractPartialShiftValue(shift, width, shiftBits);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  for (int i = width - 1; i >= 0; i--) {
    res = vc_iteExpr(vc, eqExpr(shift_ext, bvConst32(shiftBits, i)),
                     bvRightShift(expr, i), res);
  }

  // If overshifting, shift to zero
  // If overshifting, shift to zero
  ExprHandle ex =
      vc_bvLtExpr(vc, shift, bvConst32(vc_getBVLength(vc, shift), width));
  res = vc_iteExpr(vc, ex, res, bvZero(width));

  return res;
}

// arithmetic right shift by a variable amount on an expression of the specified
// width
ExprHandle STPBuilder::bvVarArithRightShift(ExprHandle expr, ExprHandle shift) {
  unsigned width = vc_getBVLength(vc, expr);

  unsigned shiftBits = 0;
  ExprHandle shift_ext = extractPartialShiftValue(shift, width, shiftBits);

  // get the sign bit to fill with
  ExprHandle signedBool = bvBoolExtract(expr, width - 1);

  // start with the result if shifting by width-1
  ExprHandle res = constructAShrByConstant(expr, width - 1, signedBool);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  // XXX more efficient to move the ite on the sign outside all exprs?
  // XXX more efficient to sign extend, right shift, then extract lower bits?
  for (int i = width - 2; i >= 0; i--) {
    res = vc_iteExpr(vc, eqExpr(shift_ext, bvConst32(shiftBits, i)),
                     constructAShrByConstant(expr, i, signedBool), res);
  }

  // If overshifting, shift to zero
  ExprHandle ex =
      vc_bvLtExpr(vc, shift, bvConst32(vc_getBVLength(vc, shift), width));
  res = vc_iteExpr(vc, ex, res, bvZero(width));
  return res;
}

ExprHandle STPBuilder::constructAShrByConstant(ExprHandle expr,
                                               unsigned shift,
                                               ExprHandle isSigned) {
  unsigned width = vc_getBVLength(vc, expr);

  if (shift==0) {
    return expr;
  } else if (shift>=width) {
    return bvZero(width); // Overshift to zero
  } else {
    return vc_iteExpr(vc,
                      isSigned,
                      ExprHandle(vc_bvConcatExpr(vc,
                                                 bvMinusOne(shift),
                                                 bvExtract(expr, width - 1, shift))),
                      bvRightShift(expr, shift));
  }
}

ExprHandle STPBuilder::constructMulByConstant(ExprHandle expr, unsigned width, uint64_t x) {
  uint64_t add, sub;
  ExprHandle res = 0;

  // expr*x == expr*(add-sub) == expr*add - expr*sub
  ComputeMultConstants64(x, add, sub);

  // legal, these would overflow completely
  add = bits64::truncateToNBits(add, width);
  sub = bits64::truncateToNBits(sub, width);

  for (int j=63; j>=0; j--) {
    uint64_t bit = 1LL << j;

    if ((add&bit) || (sub&bit)) {
      assert(!((add&bit) && (sub&bit)) && "invalid mult constants");
      ExprHandle op = bvLeftShift(expr, j);
      
      if (add&bit) {
        if (res) {
          res = vc_bvPlusExpr(vc, width, res, op);
        } else {
          res = op;
        }
      } else {
        if (res) {
          res = vc_bvMinusExpr(vc, width, res, op);
        } else {
          res = vc_bvUMinusExpr(vc, op);
        }
      }
    }
  }

  if (!res) 
    res = bvZero(width);

  return res;
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
ExprHandle STPBuilder::constructUDivByConstant(ExprHandle expr_n, unsigned width, uint64_t d) {
  assert(width==32 && "can only compute udiv constants for 32-bit division");

  // Compute the constants needed to compute n/d for constant d w/o
  // division by d.
  uint32_t mprime, sh1, sh2;
  ComputeUDivConstants32(d, mprime, sh1, sh2);
  ExprHandle expr_sh1    = bvConst32( 32, sh1);
  ExprHandle expr_sh2    = bvConst32( 32, sh2);

  // t1  = MULUH(mprime, n) = ( (uint64_t)mprime * (uint64_t)n ) >> 32
  ExprHandle expr_n_64   = vc_bvConcatExpr( vc, bvZero(32), expr_n ); //extend to 64 bits
  ExprHandle t1_64bits   = constructMulByConstant( expr_n_64, 64, (uint64_t)mprime );
  ExprHandle t1          = vc_bvExtract( vc, t1_64bits, 63, 32 ); //upper 32 bits

  // n/d = (((n - t1) >> sh1) + t1) >> sh2;
  ExprHandle n_minus_t1  = vc_bvMinusExpr( vc, width, expr_n, t1 );
  ExprHandle shift_sh1   = bvVarRightShift( n_minus_t1, expr_sh1);
  ExprHandle plus_t1     = vc_bvPlusExpr( vc, width, shift_sh1, t1 );
  ExprHandle res         = bvVarRightShift( plus_t1, expr_sh2);

  return res;
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
ExprHandle STPBuilder::constructSDivByConstant(ExprHandle expr_n, unsigned width, uint64_t d) {
  // Refactor using APInt::ms APInt::magic();
  assert(width==32 && "can only compute udiv constants for 32-bit division");

  // Compute the constants needed to compute n/d for constant d w/o division by d.
  int32_t mprime, dsign, shpost;
  ComputeSDivConstants32(d, mprime, dsign, shpost);
  ExprHandle expr_dsign   = bvConst32( 32, dsign);
  ExprHandle expr_shpost  = bvConst32( 32, shpost);

  // q0 = n + MULSH( mprime, n ) = n + (( (int64_t)mprime * (int64_t)n ) >> 32)
  int64_t mprime_64     = (int64_t)mprime;

  ExprHandle expr_n_64    = vc_bvSignExtend( vc, expr_n, 64 );
  ExprHandle mult_64      = constructMulByConstant( expr_n_64, 64, mprime_64 );
  ExprHandle mulsh        = vc_bvExtract( vc, mult_64, 63, 32 ); //upper 32-bits
  ExprHandle n_plus_mulsh = vc_bvPlusExpr( vc, width, expr_n, mulsh );

  // Improved variable arithmetic right shift: sign extend, shift,
  // extract.
  ExprHandle extend_npm   = vc_bvSignExtend( vc, n_plus_mulsh, 64 );
  ExprHandle shift_npm    = bvVarRightShift( extend_npm, expr_shpost);
  ExprHandle shift_shpost = vc_bvExtract( vc, shift_npm, 31, 0 ); //lower 32-bits

  // XSIGN(n) is -1 if n is negative, positive one otherwise
  ExprHandle is_signed    = bvBoolExtract( expr_n, 31 );
  ExprHandle neg_one      = bvMinusOne(32);
  ExprHandle xsign_of_n   = vc_iteExpr( vc, is_signed, neg_one, bvZero(32) );

  // q0 = (n_plus_mulsh >> shpost) - XSIGN(n)
  ExprHandle q0           = vc_bvMinusExpr( vc, width, shift_shpost, xsign_of_n );
  
  // n/d = (q0 ^ dsign) - dsign
  ExprHandle q0_xor_dsign = vc_bvXorExpr( vc, q0, expr_dsign );
  ExprHandle res          = vc_bvMinusExpr( vc, width, q0_xor_dsign, expr_dsign );

  return res;
}

::VCExpr STPBuilder::getInitialArray(const Array *root) {
  
  assert(root);
  ::VCExpr array_expr;
  bool hashed = _arr_hash.lookupArrayExpr(root, array_expr);
  
  if (!hashed) {
    // STP uniques arrays by name, so we make sure the name is unique by
    // using the size of the array hash as a counter.
    std::string unique_id = llvm::itostr(_arr_hash._array_hash.size());
    unsigned const uid_length = unique_id.length();
    unsigned const space = (root->name.length() > 32 - uid_length)
                               ? (32 - uid_length)
                               : root->name.length();
    std::string unique_name = root->name.substr(0, space) + unique_id;

    array_expr = buildArray(unique_name.c_str(), root->getDomain(),
                            root->getRange());

    if (root->isConstantArray()) {
      // FIXME: Flush the concrete values into STP. Ideally we would do this
      // using assertions, which is much faster, but we need to fix the caching
      // to work correctly in that case.
      for (unsigned i = 0, e = root->size; i != e; ++i) {
	::VCExpr prev = array_expr;
	array_expr = vc_writeExpr(vc, prev,
                       construct(ConstantExpr::alloc(i, root->getDomain()), 0),
                       construct(root->constantValues[i], 0));
	vc_DeleteExpr(prev);
      }
    }
    
    _arr_hash.hashArrayExpr(root, array_expr);
  }
  
  return(array_expr); 
}

ExprHandle STPBuilder::getInitialRead(const Array *root, unsigned index) {
  return vc_readExpr(vc, getInitialArray(root), bvConst32(32, index));
}

::VCExpr STPBuilder::getArrayForUpdate(const Array *root, 
                                       const UpdateNode *un) {
  if (!un) {
      return(getInitialArray(root));
  }
  else {
      // FIXME: This really needs to be non-recursive.
      ::VCExpr un_expr;
      bool hashed = _arr_hash.lookupUpdateNodeExpr(un, un_expr);
      
      if (!hashed) {
	un_expr = vc_writeExpr(vc,
                               getArrayForUpdate(root, un->next),
                               construct(un->index, 0),
                               construct(un->value, 0));
	
	_arr_hash.hashUpdateNodeExpr(un, un_expr);
      }
      
      return(un_expr);
  }
}

/** if *width_out!=1 then result is a bitvector,
    otherwise it is a bool */
ExprHandle STPBuilder::construct(ref<Expr> e, int *width_out) {
  if (!UseConstructHash || isa<ConstantExpr>(e)) {
    return constructActual(e, width_out);
  } else {
    ExprHashMap< std::pair<ExprHandle, unsigned> >::iterator it = 
      constructed.find(e);
    if (it!=constructed.end()) {
      if (width_out)
        *width_out = it->second.second;
      return it->second.first;
    } else {
      int width;
      if (!width_out) width_out = &width;
      ExprHandle res = constructActual(e, width_out);
      constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
      return res;
    }
  }
}


/** if *width_out!=1 then result is a bitvector,
    otherwise it is a bool */
ExprHandle STPBuilder::constructActual(ref<Expr> e, int *width_out) {
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

    ref<ConstantExpr> Tmp = CE;
    ExprHandle Res = bvConst64(64, Tmp->Extract(0, 64)->getZExtValue());
    while (Tmp->getWidth() > 64) {
      Tmp = Tmp->Extract(64, Tmp->getWidth()-64);
      unsigned Width = std::min(64U, Tmp->getWidth());
      Res = vc_bvConcatExpr(vc, bvConst64(Width,
                                        Tmp->Extract(0, Width)->getZExtValue()),
                            Res);
    }
    return Res;
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
    return vc_readExpr(vc,
                       getArrayForUpdate(re->updates.root, re->updates.head),
                       construct(re->index, 0));
  }
    
  case Expr::Select: {
    SelectExpr *se = cast<SelectExpr>(e);
    ExprHandle cond = construct(se->cond, 0);
    ExprHandle tExpr = construct(se->trueExpr, width_out);
    ExprHandle fExpr = construct(se->falseExpr, width_out);
    return vc_iteExpr(vc, cond, tExpr, fExpr);
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    unsigned numKids = ce->getNumKids();
    ExprHandle res = construct(ce->getKid(numKids-1), 0);
    for (int i=numKids-2; i>=0; i--) {
      res = vc_bvConcatExpr(vc, construct(ce->getKid(i), 0), res);
    }
    *width_out = ce->getWidth();
    return res;
  }

  case Expr::Extract: {
    ExtractExpr *ee = cast<ExtractExpr>(e);
    ExprHandle src = construct(ee->expr, width_out);    
    *width_out = ee->getWidth();
    if (*width_out==1) {
      return bvBoolExtract(src, ee->offset);
    } else {
      return vc_bvExtract(vc, src, ee->offset + *width_out - 1, ee->offset);
    }
  }

    // Casting

  case Expr::ZExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    ExprHandle src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth==1) {
      return vc_iteExpr(vc, src, bvOne(*width_out), bvZero(*width_out));
    } else {
      return vc_bvConcatExpr(vc, bvZero(*width_out-srcWidth), src);
    }
  }

  case Expr::SExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    ExprHandle src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth==1) {
      return vc_iteExpr(vc, src, bvMinusOne(*width_out), bvZero(*width_out));
    } else {
      return vc_bvSignExtend(vc, src, *width_out);
    }
  }

    // Arithmetic

  case Expr::Add: {
    AddExpr *ae = cast<AddExpr>(e);
    ExprHandle left = construct(ae->left, width_out);
    ExprHandle right = construct(ae->right, width_out);
    assert(*width_out!=1 && "uncanonicalized add");
    return vc_bvPlusExpr(vc, *width_out, left, right);
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    ExprHandle left = construct(se->left, width_out);
    ExprHandle right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized sub");
    return vc_bvMinusExpr(vc, *width_out, left, right);
  } 

  case Expr::Mul: {
    MulExpr *me = cast<MulExpr>(e);
    ExprHandle right = construct(me->right, width_out);
    assert(*width_out!=1 && "uncanonicalized mul");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(me->left))
      if (CE->getWidth() <= 64)
        return constructMulByConstant(right, *width_out, 
                                      CE->getZExtValue());

    ExprHandle left = construct(me->left, width_out);
    return vc_bvMultExpr(vc, *width_out, left, right);
  }

  case Expr::UDiv: {
    UDivExpr *de = cast<UDivExpr>(e);
    ExprHandle left = construct(de->left, width_out);
    assert(*width_out!=1 && "uncanonicalized udiv");
    
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();
      
        if (bits64::isPowerOfTwo(divisor)) {
          return bvRightShift(left,
                              bits64::indexOfSingleBit(divisor));
        } else if (optimizeDivides) {
          if (*width_out == 32) //only works for 32-bit division
            return constructUDivByConstant( left, *width_out, 
                                            (uint32_t) divisor);
        }
      }
    } 

    ExprHandle right = construct(de->right, width_out);
    return vc_bvDivExpr(vc, *width_out, left, right);
  }

  case Expr::SDiv: {
    SDivExpr *de = cast<SDivExpr>(e);
    ExprHandle left = construct(de->left, width_out);
    assert(*width_out!=1 && "uncanonicalized sdiv");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right))
      if (optimizeDivides) {
        llvm::APInt divisor = CE->getAPValue();
        if (divisor != llvm::APInt(CE->getWidth(),1, false /*unsigned*/) &&
            divisor != llvm::APInt(CE->getWidth(), -1, true /*signed*/))
            if (*width_out == 32) //only works for 32-bit division
               return constructSDivByConstant( left, *width_out,
                                          CE->getZExtValue(32));
      }
    // XXX need to test for proper handling of sign, not sure I
    // trust STP
    ExprHandle right = construct(de->right, width_out);
    return vc_sbvDivExpr(vc, *width_out, left, right);
  }

  case Expr::URem: {
    URemExpr *de = cast<URemExpr>(e);
    ExprHandle left = construct(de->left, width_out);
    assert(*width_out!=1 && "uncanonicalized urem");
    
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();

        if (bits64::isPowerOfTwo(divisor)) {
          unsigned bits = bits64::indexOfSingleBit(divisor);

          // special case for modding by 1 or else we bvExtract -1:0
          if (bits == 0) {
            return bvZero(*width_out);
          } else {
            return vc_bvConcatExpr(vc,
                                   bvZero(*width_out - bits),
                                   bvExtract(left, bits - 1, 0));
          }
        }

        // Use fast division to compute modulo without explicit division for
        // constant divisor.

        if (optimizeDivides) {
          if (*width_out == 32) { //only works for 32-bit division
            ExprHandle quotient = constructUDivByConstant( left, *width_out, (uint32_t)divisor );
            ExprHandle quot_times_divisor = constructMulByConstant( quotient, *width_out, divisor );
            ExprHandle rem = vc_bvMinusExpr( vc, *width_out, left, quot_times_divisor );
            return rem;
          }
        }
      }
    }
    
    ExprHandle right = construct(de->right, width_out);
    return vc_bvModExpr(vc, *width_out, left, right);
  }

  case Expr::SRem: {
    SRemExpr *de = cast<SRemExpr>(e);
    ExprHandle left = construct(de->left, width_out);
    ExprHandle right = construct(de->right, width_out);
    assert(*width_out!=1 && "uncanonicalized srem");

#if 0 //not faster per first benchmark
    if (optimizeDivides) {
      if (ConstantExpr *cre = de->right->asConstant()) {
	uint64_t divisor = cre->asUInt64;

	//use fast division to compute modulo without explicit division for constant divisor
      	if( *width_out == 32 ) { //only works for 32-bit division
	  ExprHandle quotient = constructSDivByConstant( left, *width_out, divisor );
	  ExprHandle quot_times_divisor = constructMulByConstant( quotient, *width_out, divisor );
	  ExprHandle rem = vc_bvMinusExpr( vc, *width_out, left, quot_times_divisor );
	  return rem;
	}
      }
    }
#endif

    // XXX implement my fast path and test for proper handling of sign
    return vc_sbvRemExpr(vc, *width_out, left, right);
  }

    // Bitwise

  case Expr::Not: {
    NotExpr *ne = cast<NotExpr>(e);
    ExprHandle expr = construct(ne->expr, width_out);
    if (*width_out==1) {
      return vc_notExpr(vc, expr);
    } else {
      return vc_bvNotExpr(vc, expr);
    }
  }    

  case Expr::And: {
    AndExpr *ae = cast<AndExpr>(e);
    ExprHandle left = construct(ae->left, width_out);
    ExprHandle right = construct(ae->right, width_out);
    if (*width_out==1) {
      return vc_andExpr(vc, left, right);
    } else {
      return vc_bvAndExpr(vc, left, right);
    }
  }

  case Expr::Or: {
    OrExpr *oe = cast<OrExpr>(e);
    ExprHandle left = construct(oe->left, width_out);
    ExprHandle right = construct(oe->right, width_out);
    if (*width_out==1) {
      return vc_orExpr(vc, left, right);
    } else {
      return vc_bvOrExpr(vc, left, right);
    }
  }

  case Expr::Xor: {
    XorExpr *xe = cast<XorExpr>(e);
    ExprHandle left = construct(xe->left, width_out);
    ExprHandle right = construct(xe->right, width_out);
    
    if (*width_out==1) {
      // XXX check for most efficient?
      return vc_iteExpr(vc, left, 
                        ExprHandle(vc_notExpr(vc, right)), right);
    } else {
      return vc_bvXorExpr(vc, left, right);
    }
  }

  case Expr::Shl: {
    ShlExpr *se = cast<ShlExpr>(e);
    ExprHandle left = construct(se->left, width_out);
    assert(*width_out!=1 && "uncanonicalized shl");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
      return bvLeftShift(left, (unsigned) CE->getLimitedValue());
    } else {
      int shiftWidth;
      ExprHandle amount = construct(se->right, &shiftWidth);
      return bvVarLeftShift( left, amount);
    }
  }

  case Expr::LShr: {
    LShrExpr *lse = cast<LShrExpr>(e);
    ExprHandle left = construct(lse->left, width_out);
    assert(*width_out!=1 && "uncanonicalized lshr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
      return bvRightShift(left, (unsigned) CE->getLimitedValue());
    } else {
      int shiftWidth;
      ExprHandle amount = construct(lse->right, &shiftWidth);
      return bvVarRightShift( left, amount);
    }
  }

  case Expr::AShr: {
    AShrExpr *ase = cast<AShrExpr>(e);
    ExprHandle left = construct(ase->left, width_out);
    assert(*width_out!=1 && "uncanonicalized ashr");
    
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ase->right)) {
      unsigned shift = (unsigned) CE->getLimitedValue();
      ExprHandle signedBool = bvBoolExtract(left, *width_out-1);
      return constructAShrByConstant(left, shift, signedBool);
    } else {
      int shiftWidth;
      ExprHandle amount = construct(ase->right, &shiftWidth);
      return bvVarArithRightShift( left, amount);
    }
  }

    // Comparison

  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    ExprHandle left = construct(ee->left, width_out);
    ExprHandle right = construct(ee->right, width_out);
    if (*width_out==1) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ee->left)) {
        if (CE->isTrue())
          return right;
        return vc_notExpr(vc, right);
      } else {
        return vc_iffExpr(vc, left, right);
      }
    } else {
      *width_out = 1;
      return vc_eqExpr(vc, left, right);
    }
  }

  case Expr::Ult: {
    UltExpr *ue = cast<UltExpr>(e);
    ExprHandle left = construct(ue->left, width_out);
    ExprHandle right = construct(ue->right, width_out);
    assert(*width_out!=1 && "uncanonicalized ult");
    *width_out = 1;
    return vc_bvLtExpr(vc, left, right);
  }

  case Expr::Ule: {
    UleExpr *ue = cast<UleExpr>(e);
    ExprHandle left = construct(ue->left, width_out);
    ExprHandle right = construct(ue->right, width_out);
    assert(*width_out!=1 && "uncanonicalized ule");
    *width_out = 1;
    return vc_bvLeExpr(vc, left, right);
  }

  case Expr::Slt: {
    SltExpr *se = cast<SltExpr>(e);
    ExprHandle left = construct(se->left, width_out);
    ExprHandle right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized slt");
    *width_out = 1;
    return vc_sbvLtExpr(vc, left, right);
  }

  case Expr::Sle: {
    SleExpr *se = cast<SleExpr>(e);
    ExprHandle left = construct(se->left, width_out);
    ExprHandle right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized sle");
    *width_out = 1;
    return vc_sbvLeExpr(vc, left, right);
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
    return vc_trueExpr(vc);
  }
}
#endif // ENABLE_STP
