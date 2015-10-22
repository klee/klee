//===-- Z3Builder.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Z3Builder.h"

#include "klee/Expr.h"
#include "klee/Solver.h"
#include "klee/util/Bits.h"

#include "ConstantDivision.h"
#include "SolverStats.h"

#include "llvm/Support/CommandLine.h"

#include <cstdio>

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


Z3ArrayExprHash::~Z3ArrayExprHash() {
  for (ArrayHashIter it = _array_hash.begin(); it != _array_hash.end(); ++it) {
    ::z3::expr array_expr = it->second;
    if (array_expr) {
    	// FIXME: Perhaps we need to delete array_expr; array_expr = 0;
    }
  }


  for (UpdateNodeHashConstIter it = _update_node_hash.begin();
      it != _update_node_hash.end(); ++it) {
    Z3ExprHandle un_expr = it->second;
    if (un_expr) {
      // FIXME: Perhaps we need to delete un_expr; un_expr = 0;
    }
  }
}

/***/

Z3Builder::Z3Builder() {
  tempVars[0] = buildVar("__tmpInt8", 8);
  tempVars[1] = buildVar("__tmpInt16", 16);
  tempVars[2] = buildVar("__tmpInt32", 32);
  tempVars[3] = buildVar("__tmpInt64", 64);
}

Z3Builder::~Z3Builder() {
  
}

///

/* Warning: be careful about what c_interface functions you use. Some of
   them look like they cons memory but in fact don't, which is bad when
   you call vc_DeleteExpr on them. */

::z3::expr Z3Builder::buildVar(const char *name, unsigned width) {
  // XXX don't rebuild if this stuff cons's
  ::z3::sort t = (width==1) ? ctx.bool_sort() : ctx.bv_sort(width);
  ::z3::expr res = ctx.constant(const_cast<char*>(name), t);
  return res;
}

::z3::expr Z3Builder::buildArray(const char *name, unsigned indexWidth, unsigned valueWidth) {
  // XXX don't rebuild if this stuff cons's
  ::z3::sort t1 = ctx.bv_sort(indexWidth);
  ::z3::sort t2 = ctx.bv_sort(valueWidth);
  ::z3::sort t = ctx.array_sort(t1, t2);
  ::z3::expr res = ctx.constant(const_cast<char*>(name), t);
  return res;
}

Z3ExprHandle Z3Builder::getTempVar(Expr::Width w) {
  switch (w) {
  default: assert(0 && "invalid type");
  case Expr::Int8: return tempVars[0];
  case Expr::Int16: return tempVars[1];
  case Expr::Int32: return tempVars[2];
  case Expr::Int64: return tempVars[3];
  }
}

Z3ExprHandle Z3Builder::getTrue() {
  return ctx.bool_val(true);
}

Z3ExprHandle Z3Builder::getFalse() {
  return ctx.bool_val(false);
}

Z3ExprHandle Z3Builder::bvOne(unsigned width) {
  return bvZExtConst(width, 1);
}

Z3ExprHandle Z3Builder::bvZero(unsigned width) {
  return bvZExtConst(width, 0);
}

Z3ExprHandle Z3Builder::bvMinusOne(unsigned width) {
  return bvSExtConst(width, (int64_t) -1);
}

Z3ExprHandle Z3Builder::bvConst32(unsigned width, uint32_t value) {
  return ctx.bv_val(value, width);
}

Z3ExprHandle Z3Builder::bvConst64(unsigned width, uint64_t value) {
  return ctx.bv_val((long long int) value, width);
}

Z3ExprHandle bvConcatExpr(Z3ExprHandle e1, Z3ExprHandle e2) {
  return ::z3::concat(e1, e2);
}

Z3ExprHandle Z3Builder::bvZExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  Z3ExprHandle expr = bvConst64(64, value), zero = bvConst64(64, 0);
  for (width -= 64; width > 64; width -= 64)
    expr = bvConcatExpr(zero, expr);
  return bvConcatExpr(bvConst64(width, 0), expr);
}

Z3ExprHandle Z3Builder::bvSExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  if (value >> 63) {
	  return bvConcatExpr(ctx.bv_val(-1, width - 64), bvConst64(64, value));
  }

  return bvConcatExpr(ctx.bv_val(0, width - 64), bvConst64(64, value));
}

Z3ExprHandle Z3Builder::bvBoolExtract(Z3ExprHandle expr, int bit) {
  return (((::z3::expr) bvExtract(expr, bit, bit)) == ((::z3::expr) bvOne(1)));
}

Z3ExprHandle Z3Builder::bvExtract(Z3ExprHandle expr, unsigned top, unsigned bottom) {
  return Z3ExprHandle(::z3::expr(ctx, Z3_mk_extract(ctx, top, bottom, ((::z3::expr) expr))));
}

Z3ExprHandle Z3Builder::eqExpr(Z3ExprHandle a, Z3ExprHandle b) {
  return (((::z3::expr) a) == ((::z3::expr) b));
}

// logical right shift
Z3ExprHandle Z3Builder::bvRightShift(Z3ExprHandle expr, unsigned shift) {
  unsigned width = getBVLength(expr);

  if (shift==0) {
    return expr;
  } else if (shift>=width) {
    return bvZero(width); // Overshift to zero
  } else {
    return bvConcatExpr(bvZero(shift), bvExtract(expr, width - 1, shift));
  }
}

// logical left shift
Z3ExprHandle Z3Builder::bvLeftShift(Z3ExprHandle expr, unsigned shift) {
  unsigned width = getBVLength(expr);

  if (shift==0) {
    return expr;
  } else if (shift>=width) {
    return bvZero(width); // Overshift to zero
  } else {
    return bvConcatExpr(bvExtract(expr, width - shift - 1, 0), bvZero(shift));
  }
}

// left shift by a variable amount on an expression of the specified width
Z3ExprHandle Z3Builder::bvVarLeftShift(Z3ExprHandle expr, Z3ExprHandle shift) {
  unsigned width = getBVLength(expr);
  Z3ExprHandle res = bvZero(width);

  //construct a big if-then-elif-elif-... with one case per possible shift amount
  for( int i=width-1; i>=0; i-- ) {
    res = iteExpr(eqExpr(shift, bvConst32(width, i)), bvLeftShift(expr, i), res);
  }

  // If overshifting, shift to zero
  Z3ExprHandle ex = bvLtExpr(shift, bvConst32(getBVLength(shift), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// logical right shift by a variable amount on an expression of the specified width
Z3ExprHandle Z3Builder::bvVarRightShift(Z3ExprHandle expr, Z3ExprHandle shift) {
  unsigned width = getBVLength(expr);
  Z3ExprHandle res = bvZero(width);

  //construct a big if-then-elif-elif-... with one case per possible shift amount
  for( int i=width-1; i>=0; i-- ) {
    res = iteExpr(eqExpr(shift, bvConst32(width, i)),
            	bvRightShift(expr, i), res);
  }

  // If overshifting, shift to zero
  Z3ExprHandle ex = bvLtExpr(shift, bvConst32(getBVLength(shift), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// arithmetic right shift by a variable amount on an expression of the specified width
Z3ExprHandle Z3Builder::bvVarArithRightShift(Z3ExprHandle expr, Z3ExprHandle shift) {
  unsigned width = getBVLength(expr);

  //get the sign bit to fill with
  Z3ExprHandle signedBool = bvBoolExtract(expr, width-1);

  //start with the result if shifting by width-1
  Z3ExprHandle res = constructAShrByConstant(expr, width-1, signedBool);

  //construct a big if-then-elif-elif-... with one case per possible shift amount
  // XXX more efficient to move the ite on the sign outside all exprs?
  // XXX more efficient to sign extend, right shift, then extract lower bits?
  for( int i=width-2; i>=0; i-- ) {
    res = iteExpr(eqExpr(shift, bvConst32(width,i)),
    		constructAShrByConstant(expr, i, signedBool), res);
  }

  // If overshifting, shift to zero
  Z3ExprHandle ex = bvLtExpr(shift, bvConst32(getBVLength(shift), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

Z3ExprHandle Z3Builder::bvMinusExpr(unsigned width, Z3ExprHandle minuend, Z3ExprHandle subtrahend) {
	return z3::to_expr(ctx, Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvsub(ctx, ((::z3::expr) minuend), ((::z3::expr) subtrahend))));
}

Z3ExprHandle Z3Builder::bvPlusExpr(unsigned width, Z3ExprHandle augend, Z3ExprHandle addend) {
	return z3::to_expr(ctx, Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvadd(ctx, ((::z3::expr) augend), ((::z3::expr) addend))));
}

Z3ExprHandle Z3Builder::bvMultExpr(unsigned width, Z3ExprHandle multiplacand, Z3ExprHandle multiplier) {
	return z3::to_expr(ctx, Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvmul(ctx, ((::z3::expr) multiplacand), ((::z3::expr) multiplier))));
}

Z3ExprHandle Z3Builder::bvDivExpr(unsigned width, Z3ExprHandle dividend, Z3ExprHandle divisor) {
	return z3::to_expr(ctx, Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvudiv(ctx, ((::z3::expr) dividend), ((::z3::expr) divisor))));
}

Z3ExprHandle Z3Builder::sbvDivExpr(unsigned width, Z3ExprHandle dividend, Z3ExprHandle divisor) {
	return z3::to_expr(ctx, Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvsdiv(ctx, ((::z3::expr) dividend), ((::z3::expr) divisor))));
}

Z3ExprHandle Z3Builder::bvModExpr(unsigned width, Z3ExprHandle dividend, Z3ExprHandle divisor) {
	return z3::to_expr(ctx, Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvurem(ctx, ((::z3::expr) dividend), ((::z3::expr) divisor))));
}

Z3ExprHandle Z3Builder::sbvModExpr(unsigned width, Z3ExprHandle dividend, Z3ExprHandle divisor) {
	// FIXME: An alternative is to use Z3_mk_bvsmod, need to check which one is good
	return z3::to_expr(ctx, Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvsrem(ctx, ((::z3::expr) dividend), ((::z3::expr) divisor))));
}

Z3ExprHandle Z3Builder::notExpr(Z3ExprHandle expr) {
	return z3::to_expr(ctx, Z3_mk_bvnot(ctx, Z3_mk_extract(ctx, 0, 0, ((::z3::expr) expr))));
}

Z3ExprHandle Z3Builder::bvNotExpr(Z3ExprHandle expr) {
	return z3::to_expr(ctx, Z3_mk_bvnot(ctx, ((::z3::expr) expr)));
}

Z3ExprHandle Z3Builder::andExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_bvand(ctx, Z3_mk_extract(ctx, 0, 0, ((::z3::expr) lhs)), Z3_mk_extract(ctx, 0, 0, ((::z3::expr) rhs))));
}

Z3ExprHandle Z3Builder::bvAndExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_bvand(ctx, ((::z3::expr) lhs), ((::z3::expr) rhs)));
}

Z3ExprHandle Z3Builder::orExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_bvor(ctx, Z3_mk_extract(ctx, 0, 0, ((::z3::expr) lhs)), Z3_mk_extract(ctx, 0, 0, ((::z3::expr) rhs))));
}

Z3ExprHandle Z3Builder::bvOrExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_bvor(ctx, ((::z3::expr) lhs), ((::z3::expr) rhs)));
}

Z3ExprHandle Z3Builder::iffExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_extract(ctx, 0, 0, Z3_mk_bvxor(ctx, Z3_mk_bvnot(ctx, ((::z3::expr) lhs)), ((::z3::expr) rhs))));
}

Z3ExprHandle Z3Builder::bvXorExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_xor(ctx, (::z3::expr) lhs, (::z3::expr) rhs));
}

Z3ExprHandle Z3Builder::bvSignExtend(Z3ExprHandle src, unsigned width) {
	return z3::to_expr(ctx, Z3_mk_sign_ext(ctx, width, ((::z3::expr) src)));
}

Z3ExprHandle Z3Builder::writeExpr(Z3ExprHandle array, Z3ExprHandle index, Z3ExprHandle value) {
	return z3::to_expr(ctx, Z3_mk_store(ctx, (::z3::expr) array, (::z3::expr) index, (::z3::expr) value));
}

Z3ExprHandle Z3Builder::readExpr(Z3ExprHandle array, Z3ExprHandle index) {
	return z3::to_expr(ctx, Z3_mk_select(ctx, ((::z3::expr) array), ((::z3::expr) index)));
}

Z3ExprHandle Z3Builder::iteExpr(Z3ExprHandle condition, Z3ExprHandle whenTrue, Z3ExprHandle whenFalse) {
	return z3::to_expr(ctx, Z3_mk_ite(ctx, (::z3::expr) condition, (::z3::expr) whenTrue, (::z3::expr) whenFalse));
}

int Z3Builder::getBVLength(Z3ExprHandle expr) {
	return ((::z3::expr) expr).get_sort().bv_size();
}

Z3ExprHandle Z3Builder::bvLtExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_bvult(ctx, ((::z3::expr) lhs), ((::z3::expr) rhs)));
}

Z3ExprHandle Z3Builder::bvLeExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_bvule(ctx, ((::z3::expr) lhs), ((::z3::expr) rhs)));
}

Z3ExprHandle Z3Builder::sbvLtExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_bvslt(ctx, ((::z3::expr) lhs), ((::z3::expr) rhs)));
}

Z3ExprHandle Z3Builder::sbvLeExpr(Z3ExprHandle lhs, Z3ExprHandle rhs) {
	return z3::to_expr(ctx, Z3_mk_bvsle(ctx, ((::z3::expr) lhs), ((::z3::expr) rhs)));
}

Z3ExprHandle Z3Builder::constructAShrByConstant(Z3ExprHandle expr,
                                               unsigned shift,
                                               Z3ExprHandle isSigned) {
  unsigned width = getBVLength(expr);

  if (shift==0) {
    return expr;
  } else if (shift>=width) {
    return bvZero(width); // Overshift to zero
  } else {
    return iteExpr(isSigned,
    		Z3ExprHandle(bvConcatExpr(bvMinusOne(shift), bvExtract(expr, width - 1, shift))),
    		bvRightShift(expr, shift));
  }
}

Z3ExprHandle Z3Builder::constructMulByConstant(Z3ExprHandle expr, unsigned width, uint64_t x) {
  uint64_t add, sub;
  Z3ExprHandle res;

  // expr*x == expr*(add-sub) == expr*add - expr*sub
  ComputeMultConstants64(x, add, sub);

  // legal, these would overflow completely
  add = bits64::truncateToNBits(add, width);
  sub = bits64::truncateToNBits(sub, width);

  for (int j=63; j>=0; j--) {
    uint64_t bit = 1LL << j;

    if ((add&bit) || (sub&bit)) {
      assert(!((add&bit) && (sub&bit)) && "invalid mult constants");
      Z3ExprHandle op = bvLeftShift(expr, j);
      
      if (add&bit) {
        if (res) {
          res = ((::z3::expr) res) + ((::z3::expr) op);
        } else {
          res = op;
        }
      } else {
        if (res) {
          res = ((::z3::expr) res) - ((::z3::expr) op);
        } else {
          res = ((::z3::expr) bvZero(width)) - ((::z3::expr) op);
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
Z3ExprHandle Z3Builder::constructUDivByConstant(Z3ExprHandle expr_n, unsigned width, uint64_t d) {
  assert(width==32 && "can only compute udiv constants for 32-bit division");

  // Compute the constants needed to compute n/d for constant d w/o
  // division by d.
  uint32_t mprime, sh1, sh2;
  ComputeUDivConstants32(d, mprime, sh1, sh2);
  Z3ExprHandle expr_sh1    = bvConst32( 32, sh1);
  Z3ExprHandle expr_sh2    = bvConst32( 32, sh2);

  // t1  = MULUH(mprime, n) = ( (uint64_t)mprime * (uint64_t)n ) >> 32
  Z3ExprHandle expr_n_64   = bvConcatExpr(bvZero(32), expr_n ); //extend to 64 bits
  Z3ExprHandle t1_64bits   = constructMulByConstant( expr_n_64, 64, (uint64_t)mprime );
  Z3ExprHandle t1          = ((::z3::expr) t1_64bits).extract(63, 32); //upper 32 bits

  // n/d = (((n - t1) >> sh1) + t1) >> sh2;
  Z3ExprHandle n_minus_t1  = bvMinusExpr(width, expr_n, t1);
  Z3ExprHandle shift_sh1   = bvVarRightShift( n_minus_t1, expr_sh1);
  Z3ExprHandle plus_t1     = bvPlusExpr(width, shift_sh1, t1);
  Z3ExprHandle res         = bvVarRightShift( plus_t1, expr_sh2);

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
Z3ExprHandle Z3Builder::constructSDivByConstant(Z3ExprHandle expr_n, unsigned width, uint64_t d) {
  // Refactor using APInt::ms APInt::magic();
  assert(width==32 && "can only compute udiv constants for 32-bit division");

  // Compute the constants needed to compute n/d for constant d w/o division by d.
  int32_t mprime, dsign, shpost;
  ComputeSDivConstants32(d, mprime, dsign, shpost);
  Z3ExprHandle expr_dsign   = bvConst32( 32, dsign);
  Z3ExprHandle expr_shpost  = bvConst32( 32, shpost);

  // q0 = n + MULSH( mprime, n ) = n + (( (int64_t)mprime * (int64_t)n ) >> 32)
  int64_t mprime_64     = (int64_t)mprime;

  Z3ExprHandle expr_n_64    = bvSignExtend(expr_n, 64);
  Z3ExprHandle mult_64      = constructMulByConstant( expr_n_64, 64, mprime_64 );
  Z3ExprHandle mulsh        = ((::z3::expr) mult_64).extract(63, 32 ); //upper 32-bits
  Z3ExprHandle n_plus_mulsh = bvPlusExpr( width, expr_n, mulsh );

  // Improved variable arithmetic right shift: sign extend, shift,
  // extract.
  Z3ExprHandle extend_npm   = bvSignExtend(n_plus_mulsh, 64);
  Z3ExprHandle shift_npm    = bvVarRightShift( extend_npm, expr_shpost);
  Z3ExprHandle shift_shpost = ((::z3::expr) shift_npm).extract(31, 0); //lower 32-bits

  // XSIGN(n) is -1 if n is negative, positive one otherwise
  Z3ExprHandle is_signed    = bvBoolExtract( expr_n, 31 );
  Z3ExprHandle neg_one      = bvMinusOne(32);
  Z3ExprHandle xsign_of_n   = iteExpr( is_signed, neg_one, bvZero(32) );

  // q0 = (n_plus_mulsh >> shpost) - XSIGN(n)
  Z3ExprHandle q0           = bvMinusExpr( width, shift_shpost, xsign_of_n );
  
  // n/d = (q0 ^ dsign) - dsign
  Z3ExprHandle q0_xor_dsign = bvXorExpr( q0, expr_dsign );
  Z3ExprHandle res          = bvMinusExpr( width, q0_xor_dsign, expr_dsign );

  return res;
}

Z3ExprHandle Z3Builder::getInitialArray(const Array *root) {
  
  assert(root);
  Z3ExprHandle array_expr;
  bool hashed = _arr_hash.lookupArrayExpr(root, array_expr);
  
  if (!hashed) {
    // STP uniques arrays by name, so we make sure the name is unique by
    // including the address.
    char buf[32];
    unsigned const addrlen = sprintf(buf, "_%p", (const void*)root) + 1; // +1 for null-termination
    unsigned const space = (root->name.length() > 32 - addrlen)?(32 - addrlen):root->name.length();
    memmove(buf + space, buf, addrlen); // moving the address part to the end
    memcpy(buf, root->name.c_str(), space); // filling out the name part
    
    array_expr = buildArray(buf, root->getDomain(), root->getRange());
    
    if (root->isConstantArray()) {
    	// FIXME: Flush the concrete values into STP. Ideally we would do this
    	// using assertions, which is much faster, but we need to fix the caching
    	// to work correctly in that case.
    	for (unsigned i = 0, e = root->size; i != e; ++i) {
    		::z3::expr prev = array_expr;
    		array_expr = writeExpr(prev,
    				construct(ConstantExpr::alloc(i, root->getDomain()), 0),
    				construct(root->constantValues[i], 0));
    	}
    }

    _arr_hash.hashArrayExpr(root, array_expr);
  }
  
  return(array_expr); 
}

Z3ExprHandle Z3Builder::getInitialRead(const Array *root, unsigned index) {
  return readExpr(getInitialArray(root), bvConst32(32, index));
}

::z3::expr Z3Builder::getArrayForUpdate(const Array *root,
                                       const UpdateNode *un) {
  if (!un) {
      return(getInitialArray(root));
  }
  else {
	  // FIXME: This really needs to be non-recursive.
	  Z3ExprHandle un_expr;
	  bool hashed = _arr_hash.lookupUpdateNodeExpr(un, un_expr);

	  if (!hashed) {
		  un_expr = writeExpr(getArrayForUpdate(root, un->next),
				  construct(un->index, 0),
				  construct(un->value, 0));

		  _arr_hash.hashUpdateNodeExpr(un, un_expr);
	  }

	  return(un_expr);
  }
}

/** if *width_out!=1 then result is a bitvector,
    otherwise it is a bool */
Z3ExprHandle Z3Builder::construct(ref<Expr> e, int *width_out) {
  if (!UseConstructHash || isa<ConstantExpr>(e)) {
    return constructActual(e, width_out);
  } else {
    ExprHashMap< std::pair<Z3ExprHandle, unsigned> >::iterator it =
      constructed.find(e);
    if (it!=constructed.end()) {
      if (width_out)
        *width_out = it->second.second;
      return it->second.first;
    } else {
      int width;
      if (!width_out) width_out = &width;
      Z3ExprHandle res = constructActual(e, width_out);
      constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
      return res;
    }
  }
}


/** if *width_out!=1 then result is a bitvector,
    otherwise it is a bool */
Z3ExprHandle Z3Builder::constructActual(ref<Expr> e, int *width_out) {
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
    Z3ExprHandle Res = bvConst64(64, Tmp->Extract(0, 64)->getZExtValue());
    while (Tmp->getWidth() > 64) {
    	Tmp = Tmp->Extract(64, Tmp->getWidth()-64);
    	unsigned Width = std::min(64U, Tmp->getWidth());
    	Res = bvConcatExpr(bvConst64(Width,
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
    return readExpr(getArrayForUpdate(re->updates.root, re->updates.head),
                    construct(re->index, 0));
  }
    
  case Expr::Select: {
    SelectExpr *se = cast<SelectExpr>(e);
    Z3ExprHandle cond = construct(se->cond, 0);
    Z3ExprHandle tExpr = construct(se->trueExpr, width_out);
    Z3ExprHandle fExpr = construct(se->falseExpr, width_out);
    return iteExpr(cond, tExpr, fExpr);
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    unsigned numKids = ce->getNumKids();
    Z3ExprHandle res = construct(ce->getKid(numKids-1), 0);
    for (int i=numKids-2; i>=0; i--) {
      res = bvConcatExpr(construct(ce->getKid(i), 0), res);
    }
    *width_out = ce->getWidth();
    return res;
  }

  case Expr::Extract: {
    ExtractExpr *ee = cast<ExtractExpr>(e);
    Z3ExprHandle src = construct(ee->expr, width_out);
    *width_out = ee->getWidth();
    if (*width_out==1) {
      return bvBoolExtract(src, ee->offset);
    } else {
      return ((::z3::expr) src).extract(ee->offset + *width_out - 1, ee->offset);
    }
  }

    // Casting

  case Expr::ZExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    Z3ExprHandle src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth==1) {
      return iteExpr(src, bvOne(*width_out), bvZero(*width_out));
    } else {
      return bvConcatExpr(bvZero(*width_out-srcWidth), src);
    }
  }

  case Expr::SExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    Z3ExprHandle src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth==1) {
      return iteExpr(src, bvMinusOne(*width_out), bvZero(*width_out));
    } else {
      return bvSignExtend(src, *width_out);
    }
  }

    // Arithmetic

  case Expr::Add: {
    AddExpr *ae = cast<AddExpr>(e);
    Z3ExprHandle left = construct(ae->left, width_out);
    Z3ExprHandle right = construct(ae->right, width_out);
    assert(*width_out!=1 && "uncanonicalized add");
    return bvPlusExpr(*width_out, left, right);
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    Z3ExprHandle left = construct(se->left, width_out);
    Z3ExprHandle right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized sub");
    return bvMinusExpr(*width_out, left, right);
  } 

  case Expr::Mul: {
	  MulExpr *me = cast<MulExpr>(e);
	  Z3ExprHandle right = construct(me->right, width_out);
	  assert(*width_out!=1 && "uncanonicalized mul");

	  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(me->left))
		  if (CE->getWidth() <= 64)
			  return constructMulByConstant(right, *width_out,
					  CE->getZExtValue());

	  Z3ExprHandle left = construct(me->left, width_out);
	  return bvMultExpr(*width_out, left, right);
  }

  case Expr::UDiv: {
	  UDivExpr *de = cast<UDivExpr>(e);
	  Z3ExprHandle left = construct(de->left, width_out);
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

    Z3ExprHandle right = construct(de->right, width_out);
    return bvDivExpr(*width_out, left, right);
  }

  case Expr::SDiv: {
    SDivExpr *de = cast<SDivExpr>(e);
    Z3ExprHandle left = construct(de->left, width_out);
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
    Z3ExprHandle right = construct(de->right, width_out);
    return sbvDivExpr(*width_out, left, right);
  }

  case Expr::URem: {
    URemExpr *de = cast<URemExpr>(e);
    Z3ExprHandle left = construct(de->left, width_out);
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
            return bvConcatExpr(bvZero(*width_out - bits),
                                   bvExtract(left, bits - 1, 0));
          }
        }

        // Use fast division to compute modulo without explicit division for
        // constant divisor.

        if (optimizeDivides) {
          if (*width_out == 32) { //only works for 32-bit division
            Z3ExprHandle quotient = constructUDivByConstant( left, *width_out, (uint32_t)divisor );
            Z3ExprHandle quot_times_divisor = constructMulByConstant( quotient, *width_out, divisor );
            Z3ExprHandle rem = bvMinusExpr( *width_out, left, quot_times_divisor );
            return rem;
          }
        }
      }
    }
    
    Z3ExprHandle right = construct(de->right, width_out);
    return bvModExpr(*width_out, left, right);
  }

  case Expr::SRem: {
    SRemExpr *de = cast<SRemExpr>(e);
    Z3ExprHandle left = construct(de->left, width_out);
    Z3ExprHandle right = construct(de->right, width_out);
    assert(*width_out!=1 && "uncanonicalized srem");

#if 0 //not faster per first benchmark
    if (optimizeDivides) {
      if (ConstantExpr *cre = de->right->asConstant()) {
	uint64_t divisor = cre->asUInt64;

	//use fast division to compute modulo without explicit division for constant divisor
      	if( *width_out == 32 ) { //only works for 32-bit division
	  Z3ExprHandle quotient = constructSDivByConstant( left, *width_out, divisor );
	  Z3ExprHandle quot_times_divisor = constructMulByConstant( quotient, *width_out, divisor );
	  Z3ExprHandle rem = vc_bvMinusExpr( vc, *width_out, left, quot_times_divisor );
	  return rem;
	}
      }
    }
#endif

    // XXX implement my fast path and test for proper handling of sign
    return sbvModExpr(*width_out, left, right);
  }

    // Bitwise

  case Expr::Not: {
    NotExpr *ne = cast<NotExpr>(e);
    Z3ExprHandle expr = construct(ne->expr, width_out);
    if (*width_out==1) {
      return notExpr(expr);
    } else {
      return bvNotExpr(expr);
    }
  }    

  case Expr::And: {
    AndExpr *ae = cast<AndExpr>(e);
    Z3ExprHandle left = construct(ae->left, width_out);
    Z3ExprHandle right = construct(ae->right, width_out);
    if (*width_out==1) {
      return andExpr(left, right);
    } else {
      return bvAndExpr(left, right);
    }
  }

  case Expr::Or: {
    OrExpr *oe = cast<OrExpr>(e);
    Z3ExprHandle left = construct(oe->left, width_out);
    Z3ExprHandle right = construct(oe->right, width_out);
    if (*width_out==1) {
      return orExpr(left, right);
    } else {
      return bvOrExpr(left, right);
    }
  }

  case Expr::Xor: {
    XorExpr *xe = cast<XorExpr>(e);
    Z3ExprHandle left = construct(xe->left, width_out);
    Z3ExprHandle right = construct(xe->right, width_out);
    
    if (*width_out==1) {
      // XXX check for most efficient?
      return iteExpr(left,
                        Z3ExprHandle(notExpr(right)), right);
    } else {
      return bvXorExpr(left, right);
    }
  }

  case Expr::Shl: {
    ShlExpr *se = cast<ShlExpr>(e);
    Z3ExprHandle left = construct(se->left, width_out);
    assert(*width_out!=1 && "uncanonicalized shl");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
      return bvLeftShift(left, (unsigned) CE->getLimitedValue());
    } else {
      int shiftWidth;
      Z3ExprHandle amount = construct(se->right, &shiftWidth);
      return bvVarLeftShift( left, amount);
    }
  }

  case Expr::LShr: {
    LShrExpr *lse = cast<LShrExpr>(e);
    Z3ExprHandle left = construct(lse->left, width_out);
    assert(*width_out!=1 && "uncanonicalized lshr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
      return bvRightShift(left, (unsigned) CE->getLimitedValue());
    } else {
      int shiftWidth;
      Z3ExprHandle amount = construct(lse->right, &shiftWidth);
      return bvVarRightShift( left, amount);
    }
  }

  case Expr::AShr: {
    AShrExpr *ase = cast<AShrExpr>(e);
    Z3ExprHandle left = construct(ase->left, width_out);
    assert(*width_out!=1 && "uncanonicalized ashr");
    
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ase->right)) {
      unsigned shift = (unsigned) CE->getLimitedValue();
      Z3ExprHandle signedBool = bvBoolExtract(left, *width_out-1);
      return constructAShrByConstant(left, shift, signedBool);
    } else {
      int shiftWidth;
      Z3ExprHandle amount = construct(ase->right, &shiftWidth);
      return bvVarArithRightShift( left, amount);
    }
  }

    // Comparison

  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    Z3ExprHandle left = construct(ee->left, width_out);
    Z3ExprHandle right = construct(ee->right, width_out);
    if (*width_out==1) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ee->left)) {
        if (CE->isTrue())
          return right;
        return notExpr(right);
      } else {
        return iffExpr(left, right);
      }
    } else {
      *width_out = 1;
      return eqExpr(left, right);
    }
  }

  case Expr::Ult: {
    UltExpr *ue = cast<UltExpr>(e);
    Z3ExprHandle left = construct(ue->left, width_out);
    Z3ExprHandle right = construct(ue->right, width_out);
    assert(*width_out!=1 && "uncanonicalized ult");
    *width_out = 1;
    return bvLtExpr(left, right);
  }

  case Expr::Ule: {
    UleExpr *ue = cast<UleExpr>(e);
    Z3ExprHandle left = construct(ue->left, width_out);
    Z3ExprHandle right = construct(ue->right, width_out);
    assert(*width_out!=1 && "uncanonicalized ule");
    *width_out = 1;
    return bvLeExpr(left, right);
  }

  case Expr::Slt: {
    SltExpr *se = cast<SltExpr>(e);
    Z3ExprHandle left = construct(se->left, width_out);
    Z3ExprHandle right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized slt");
    *width_out = 1;
    return sbvLtExpr(left, right);
  }

  case Expr::Sle: {
    SleExpr *se = cast<SleExpr>(e);
    Z3ExprHandle left = construct(se->left, width_out);
    Z3ExprHandle right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized sle");
    *width_out = 1;
    return sbvLeExpr(left, right);
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
    return ctx.bv_val(1, 1);
  }
}


