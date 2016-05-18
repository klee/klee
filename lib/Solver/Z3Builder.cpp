//===-- Z3Builder.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "klee/Expr.h"
#include "klee/Solver.h"
#include "klee/SolverStats.h"
#include "klee/util/Bits.h"
#include "klee/SolverStats.h"

#include "ConstantDivision.h"
#include "Z3Builder.h"

#ifdef SUPPORT_Z3

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
  UseConstructHashZ3("use-construct-hash-z3",
                   llvm::cl::desc("Use hash-consing during Z3 query construction."),
                   llvm::cl::init(true));
}

///


Z3ArrayExprHash::~Z3ArrayExprHash() {
  for (ArrayHashIter it = _array_hash.begin(); it != _array_hash.end(); ++it) {
    Z3_ast array_expr = it->second;
    if (array_expr) {
    	// FIXME: Perhaps we need to delete array_expr; array_expr = 0;
    }
  }


  for (UpdateNodeHashConstIter it = _update_node_hash.begin();
      it != _update_node_hash.end(); ++it) {
    Z3_ast un_expr = it->second;
    if (un_expr) {
      // FIXME: Perhaps we need to delete un_expr; un_expr = 0;
    }
  }
}

/***/
void customZ3ErrorHandler(Z3_context c, Z3_error_code e) {
  llvm::errs() << "Incorrect use of Z3: " << Z3_get_error_msg(c, e) << "\n";
  // TODO: Think of a better way to terminate
  exit(1);
}

Z3Builder::Z3Builder() : quantificationContext(0) {
  Z3_config cfg = Z3_mk_config();
  Z3_set_param_value(cfg, "unsat-core", "true");
  ctx = Z3_mk_context(cfg);
  Z3_set_error_handler(ctx, customZ3ErrorHandler);
  Z3_del_config(cfg);

  tempVars[0] = buildVar("__tmpInt8", 8);
  tempVars[1] = buildVar("__tmpInt16", 16);
  tempVars[2] = buildVar("__tmpInt32", 32);
  tempVars[3] = buildVar("__tmpInt64", 64);
}

Z3Builder::~Z3Builder() {
  Z3_del_context(ctx);
}

///

/* Warning: be careful about what c_interface functions you use. Some of
   them look like they cons memory but in fact don't, which is bad when
   you call vc_DeleteExpr on them. */

Z3_ast Z3Builder::buildVar(const char *name, unsigned width) {
  // XXX don't rebuild if this stuff cons's
  Z3_sort t = (width==1) ? Z3_mk_bool_sort(ctx) : Z3_mk_bv_sort(ctx, width);
  Z3_symbol s  = Z3_mk_string_symbol(ctx, const_cast<char *>(name));
  return Z3_mk_const(ctx, s, t);
}

Z3_ast Z3Builder::buildArray(const char *name, unsigned indexWidth, unsigned valueWidth) {
  // XXX don't rebuild if this stuff cons's
  Z3_sort t1 = Z3_mk_bv_sort(ctx, indexWidth);
  Z3_sort t2 = Z3_mk_bv_sort(ctx, valueWidth);
  Z3_sort t = Z3_mk_array_sort(ctx, t1, t2);
  Z3_symbol s = Z3_mk_string_symbol(ctx, const_cast<char *>(name));
  return Z3_mk_const(ctx, s, t);
}

Z3_ast Z3Builder::getTempVar(Expr::Width w) {
  switch (w) {
  default: assert(0 && "invalid type");
  case Expr::Int8: return tempVars[0];
  case Expr::Int16: return tempVars[1];
  case Expr::Int32: return tempVars[2];
  case Expr::Int64: return tempVars[3];
  }
}

/**
 * Make 1-bit bitvector whose only element is 1.
 */
Z3_ast Z3Builder::getTrue() {
	return Z3_mk_true(ctx);
}

/**
 * Make 1-bit bitvector whose only element is 0.
 */
Z3_ast Z3Builder::getFalse() {
	return Z3_mk_false(ctx);
}

Z3_ast Z3Builder::bvOne(unsigned width) {
  return bvZExtConst(width, 1);
}

Z3_ast Z3Builder::bvZero(unsigned width) {
  return bvZExtConst(width, 0);
}

Z3_ast Z3Builder::bvMinusOne(unsigned width) {
  return bvSExtConst(width, (int64_t) -1);
}

Z3_ast Z3Builder::bvConst32(unsigned width, uint32_t value) {
	Z3_sort t = Z3_mk_bv_sort(ctx, width);
	return Z3_mk_unsigned_int(ctx, value, t);
}

Z3_ast Z3Builder::bvConst64(unsigned width, uint64_t value) {
	Z3_sort t = Z3_mk_bv_sort(ctx, width);
	return Z3_mk_unsigned_int64(ctx, value, t);
}

Z3_ast Z3Builder::bvZExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  Z3_ast expr = bvConst64(64, value), zero = bvConst64(64, 0);
  for (width -= 64; width > 64; width -= 64)
    expr = Z3_mk_concat(ctx, zero, expr);
  return Z3_mk_concat(ctx, bvConst64(width, 0), expr);
}

Z3_ast Z3Builder::bvSExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  Z3_sort t = Z3_mk_bv_sort(ctx, width - 64);
  if (value >> 63) {
	  Z3_ast r = Z3_mk_int64(ctx, -1, t);
	  return Z3_mk_concat(ctx, r, bvConst64(64, value));
  }

  Z3_ast r = Z3_mk_int64(ctx, 0, t);
  return Z3_mk_concat(ctx, r, bvConst64(64, value));
}

Z3_ast Z3Builder::bvBoolExtract(Z3_ast expr, int bit) {
  return Z3_mk_eq(ctx, bvExtract(expr, bit, bit), bvOne(1));
}

Z3_ast Z3Builder::bvExtract(Z3_ast expr, unsigned top, unsigned bottom) {
  return Z3_mk_extract(ctx, top, bottom, expr);
}

Z3_ast Z3Builder::eqExpr(Z3_ast a, Z3_ast b) {
  return Z3_mk_eq(ctx, a, b);
}

// logical right shift
Z3_ast Z3Builder::bvRightShift(Z3_ast expr, unsigned shift) {
  unsigned width = getBVLength(expr);

  if (shift==0) {
    return expr;
  } else if (shift>=width) {
    return bvZero(width); // Overshift to zero
  } else {
    return Z3_mk_concat(ctx, bvZero(shift), bvExtract(expr, width - 1, shift));
  }
}

// logical left shift
Z3_ast Z3Builder::bvLeftShift(Z3_ast expr, unsigned shift) {
  unsigned width = getBVLength(expr);

  if (shift==0) {
    return expr;
  } else if (shift>=width) {
    return bvZero(width); // Overshift to zero
  } else {
    return Z3_mk_concat(ctx, bvExtract(expr, width - shift - 1, 0), bvZero(shift));
  }
}

// left shift by a variable amount on an expression of the specified width
Z3_ast Z3Builder::bvVarLeftShift(Z3_ast expr, Z3_ast shift) {
  unsigned width = getBVLength(expr);
  Z3_ast res = bvZero(width);

  //construct a big if-then-elif-elif-... with one case per possible shift amount
  for( int i=width-1; i>=0; i-- ) {
    res = iteExpr(eqExpr(shift, bvConst32(width, i)), bvLeftShift(expr, i), res);
  }

  // If overshifting, shift to zero
  Z3_ast ex = bvLtExpr(shift, bvConst32(getBVLength(shift), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// logical right shift by a variable amount on an expression of the specified width
Z3_ast Z3Builder::bvVarRightShift(Z3_ast expr, Z3_ast shift) {
  unsigned width = getBVLength(expr);
  Z3_ast res = bvZero(width);

  //construct a big if-then-elif-elif-... with one case per possible shift amount
  for( int i=width-1; i>=0; i-- ) {
    res = iteExpr(eqExpr(shift, bvConst32(width, i)),
            	bvRightShift(expr, i), res);
  }

  // If overshifting, shift to zero
  Z3_ast ex = bvLtExpr(shift, bvConst32(getBVLength(shift), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// arithmetic right shift by a variable amount on an expression of the specified width
Z3_ast Z3Builder::bvVarArithRightShift(Z3_ast expr, Z3_ast shift) {
  unsigned width = getBVLength(expr);

  //get the sign bit to fill with
  Z3_ast signedBool = bvBoolExtract(expr, width-1);

  //start with the result if shifting by width-1
  Z3_ast res = constructAShrByConstant(expr, width-1, signedBool);

  //construct a big if-then-elif-elif-... with one case per possible shift amount
  // XXX more efficient to move the ite on the sign outside all exprs?
  // XXX more efficient to sign extend, right shift, then extract lower bits?
  for( int i=width-2; i>=0; i-- ) {
    res = iteExpr(eqExpr(shift, bvConst32(width,i)),
    		constructAShrByConstant(expr, i, signedBool), res);
  }

  // If overshifting, shift to zero
  Z3_ast ex = bvLtExpr(shift, bvConst32(getBVLength(shift), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

Z3_ast Z3Builder::bvMinusExpr(unsigned width, Z3_ast minuend, Z3_ast subtrahend) {
	return Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvsub(ctx, minuend, subtrahend));
}

Z3_ast Z3Builder::bvPlusExpr(unsigned width, Z3_ast augend, Z3_ast addend) {
	return Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvadd(ctx, augend, addend));
}

Z3_ast Z3Builder::bvMultExpr(unsigned width, Z3_ast multiplacand, Z3_ast multiplier) {
	return Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvmul(ctx, multiplacand, multiplier));
}

Z3_ast Z3Builder::bvDivExpr(unsigned width, Z3_ast dividend, Z3_ast divisor) {
	return Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvudiv(ctx, dividend, divisor));
}

Z3_ast Z3Builder::sbvDivExpr(unsigned width, Z3_ast dividend, Z3_ast divisor) {
	return Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvsdiv(ctx, dividend, divisor));
}

Z3_ast Z3Builder::bvModExpr(unsigned width, Z3_ast dividend, Z3_ast divisor) {
	return Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvurem(ctx, dividend, divisor));
}

Z3_ast Z3Builder::sbvModExpr(unsigned width, Z3_ast dividend, Z3_ast divisor) {
	// FIXME: An alternative is to use Z3_mk_bvsmod, need to check which one is good
	return Z3_mk_extract(ctx, width - 1, 0, Z3_mk_bvsrem(ctx, dividend, divisor));
}

Z3_ast Z3Builder::notExpr(Z3_ast expr) {
	return Z3_mk_not(ctx, expr);
}

Z3_ast Z3Builder::bvNotExpr(Z3_ast expr) {
	return Z3_mk_bvnot(ctx, expr);
}

Z3_ast Z3Builder::andExpr(Z3_ast lhs, Z3_ast rhs) {
	Z3_ast args[2] = { lhs, rhs };
	return Z3_mk_and(ctx, 2, args);
}

Z3_ast Z3Builder::bvAndExpr(Z3_ast lhs, Z3_ast rhs) {
	return Z3_mk_bvand(ctx, lhs, rhs);
}

Z3_ast Z3Builder::orExpr(Z3_ast lhs, Z3_ast rhs) {
	Z3_ast args[2] = { lhs, rhs };
	return Z3_mk_or(ctx, 2, args);
}

Z3_ast Z3Builder::bvOrExpr(Z3_ast lhs, Z3_ast rhs) {
	return Z3_mk_bvor(ctx, lhs, rhs);
}

Z3_ast Z3Builder::iffExpr(Z3_ast lhs, Z3_ast rhs) {
	return Z3_mk_extract(ctx, 0, 0, Z3_mk_bvxor(ctx, Z3_mk_bvnot(ctx, lhs), rhs));
}

Z3_ast Z3Builder::bvXorExpr(Z3_ast lhs, Z3_ast rhs) {
	return Z3_mk_bvxor(ctx, lhs, rhs);
}

Z3_ast Z3Builder::bvSignExtend(Z3_ast src, unsigned width) {
	unsigned src_width = Z3_get_bv_sort_size(ctx, Z3_get_sort(ctx, src));
	assert(src_width <= width && "attempted to extend longer data");

	return Z3_mk_sign_ext(ctx, width - src_width, src);
}

Z3_ast Z3Builder::writeExpr(Z3_ast array, Z3_ast index, Z3_ast value) {
	return Z3_mk_store(ctx, array, index, value);
}

Z3_ast Z3Builder::readExpr(Z3_ast array, Z3_ast index) {
	return Z3_mk_select(ctx, array, index);
}

Z3_ast Z3Builder::iteExpr(Z3_ast condition, Z3_ast whenTrue, Z3_ast whenFalse) {
	return Z3_mk_ite(ctx, condition, whenTrue, whenFalse);
}

int Z3Builder::getBVLength(Z3_ast expr) {
	return Z3_get_bv_sort_size(ctx, Z3_get_sort(ctx, expr));
}

Z3_ast Z3Builder::bvLtExpr(Z3_ast lhs, Z3_ast rhs) {
	return Z3_mk_bvult(ctx, lhs, rhs);
}

Z3_ast Z3Builder::bvLeExpr(Z3_ast lhs, Z3_ast rhs) {
	return Z3_mk_bvule(ctx, lhs, rhs);
}

Z3_ast Z3Builder::sbvLtExpr(Z3_ast lhs, Z3_ast rhs) {
	return Z3_mk_bvslt(ctx, lhs, rhs);
}

Z3_ast Z3Builder::sbvLeExpr(Z3_ast lhs, Z3_ast rhs) {
	return Z3_mk_bvsle(ctx, lhs, rhs);
}

Z3_ast Z3Builder::existsExpr(Z3_ast body) {
  return Z3_mk_exists(ctx, 0, 0, 0, getQuantificationSize(),
                      getQuantificationSorts(), getQuantificationSymbols(),
                      body);
}

Z3_ast Z3Builder::constructAShrByConstant(Z3_ast expr,
                                               unsigned shift,
                                               Z3_ast isSigned) {
  unsigned width = getBVLength(expr);

  if (shift==0) {
    return expr;
  } else if (shift>=width) {
    return bvZero(width); // Overshift to zero
  } else {
    return iteExpr(isSigned,
    		Z3_mk_concat(ctx, bvMinusOne(shift), bvExtract(expr, width - 1, shift)),
    		bvRightShift(expr, shift));
  }
}

Z3_ast Z3Builder::constructMulByConstant(Z3_ast expr, unsigned width, uint64_t x) {
  uint64_t add, sub;
  Z3_ast res = 0;

  // expr*x == expr*(add-sub) == expr*add - expr*sub
  ComputeMultConstants64(x, add, sub);

  // legal, these would overflow completely
  add = bits64::truncateToNBits(add, width);
  sub = bits64::truncateToNBits(sub, width);

  for (int j=63; j>=0; j--) {
    uint64_t bit = 1LL << j;

    if ((add&bit) || (sub&bit)) {
      assert(!((add&bit) && (sub&bit)) && "invalid mult constants");
      Z3_ast op = bvLeftShift(expr, j);
      
      if (add&bit) {
        if (res) {
          res = Z3_mk_bvadd(ctx, res, op);
        } else {
          res = op;
        }
      } else {
        if (res) {
          res = Z3_mk_bvsub(ctx, res, op);
        } else {
          res = Z3_mk_bvsub(ctx, bvZero(width), op);
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
Z3_ast Z3Builder::constructUDivByConstant(Z3_ast expr_n, unsigned width, uint64_t d) {
  assert(width==32 && "can only compute udiv constants for 32-bit division");

  // Compute the constants needed to compute n/d for constant d w/o
  // division by d.
  uint32_t mprime, sh1, sh2;
  ComputeUDivConstants32(d, mprime, sh1, sh2);
  Z3_ast expr_sh1    = bvConst32( 32, sh1);
  Z3_ast expr_sh2    = bvConst32( 32, sh2);

  // t1  = MULUH(mprime, n) = ( (uint64_t)mprime * (uint64_t)n ) >> 32
  Z3_ast expr_n_64   = Z3_mk_concat(ctx, bvZero(32), expr_n ); //extend to 64 bits
  Z3_ast t1_64bits   = constructMulByConstant( expr_n_64, 64, (uint64_t)mprime );
  Z3_ast t1          = bvExtract(t1_64bits, 63, 32); //upper 32 bits

  // n/d = (((n - t1) >> sh1) + t1) >> sh2;
  Z3_ast n_minus_t1  = bvMinusExpr(width, expr_n, t1);
  Z3_ast shift_sh1   = bvVarRightShift( n_minus_t1, expr_sh1);
  Z3_ast plus_t1     = bvPlusExpr(width, shift_sh1, t1);
  Z3_ast res         = bvVarRightShift( plus_t1, expr_sh2);

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
Z3_ast Z3Builder::constructSDivByConstant(Z3_ast expr_n, unsigned width, uint64_t d) {
  // Refactor using APInt::ms APInt::magic();
  assert(width==32 && "can only compute udiv constants for 32-bit division");

  // Compute the constants needed to compute n/d for constant d w/o division by d.
  int32_t mprime, dsign, shpost;
  ComputeSDivConstants32(d, mprime, dsign, shpost);
  Z3_ast expr_dsign   = bvConst32( 32, dsign);
  Z3_ast expr_shpost  = bvConst32( 32, shpost);

  // q0 = n + MULSH( mprime, n ) = n + (( (int64_t)mprime * (int64_t)n ) >> 32)
  int64_t mprime_64     = (int64_t)mprime;

  Z3_ast expr_n_64    = bvSignExtend(expr_n, 64);
  Z3_ast mult_64      = constructMulByConstant( expr_n_64, 64, mprime_64 );
  Z3_ast mulsh        = bvExtract(mult_64, 63, 32 ); //upper 32-bits
  Z3_ast n_plus_mulsh = bvPlusExpr( width, expr_n, mulsh );

  // Improved variable arithmetic right shift: sign extend, shift,
  // extract.
  Z3_ast extend_npm   = bvSignExtend(n_plus_mulsh, 64);
  Z3_ast shift_npm    = bvVarRightShift( extend_npm, expr_shpost);
  Z3_ast shift_shpost = bvExtract(shift_npm, 31, 0); //lower 32-bits

  // XSIGN(n) is -1 if n is negative, positive one otherwise
  Z3_ast is_signed    = bvBoolExtract( expr_n, 31 );
  Z3_ast neg_one      = bvMinusOne(32);
  Z3_ast xsign_of_n   = iteExpr( is_signed, neg_one, bvZero(32) );

  // q0 = (n_plus_mulsh >> shpost) - XSIGN(n)
  Z3_ast q0           = bvMinusExpr( width, shift_shpost, xsign_of_n );
  
  // n/d = (q0 ^ dsign) - dsign
  Z3_ast q0_xor_dsign = bvXorExpr( q0, expr_dsign );
  Z3_ast res          = bvMinusExpr( width, q0_xor_dsign, expr_dsign );

  return res;
}

Z3_ast Z3Builder::getInitialArray(const Array *root) {
  
  assert(root);
  Z3_ast array_expr;
  bool hashed = _arr_hash.lookupArrayExpr(root, array_expr);
  
  if (!hashed) {
    // In case this array is bound
    Z3_ast boundVar =
        (quantificationContext ? quantificationContext->getBoundVar(root->name)
                               : 0);
    if (boundVar)
      return boundVar;

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
    		Z3_ast prev = array_expr;
    		array_expr = writeExpr(prev,
    				construct(ConstantExpr::alloc(i, root->getDomain()), 0),
    				construct(root->constantValues[i], 0));
    	}
    }

    _arr_hash.hashArrayExpr(root, array_expr);
  }
  
  return(array_expr); 
}

Z3_ast Z3Builder::getInitialRead(const Array *root, unsigned index) {
  return readExpr(getInitialArray(root), bvConst32(32, index));
}

Z3_ast Z3Builder::getArrayForUpdate(const Array *root,
                                       const UpdateNode *un) {
  if (!un) {
      return(getInitialArray(root));
  }
  else {
	  // FIXME: This really needs to be non-recursive.
	  Z3_ast un_expr;
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

void
Z3Builder::pushQuantificationContext(std::set<const Array *> existentials) {
  quantificationContext =
      new QuantificationContext(ctx, existentials, quantificationContext);
}

void Z3Builder::popQuantificationContext() {
  QuantificationContext *tmp = quantificationContext;
  quantificationContext = tmp->getParent();
  delete tmp;
}

/** if *width_out!=1 then result is a bitvector,
    otherwise it is a bool */
Z3_ast Z3Builder::construct(ref<Expr> e, int *width_out) {
  if (!UseConstructHashZ3 || isa<ConstantExpr>(e)) {
    return constructActual(e, width_out);
  } else {
    ExprHashMap< std::pair<Z3_ast, unsigned> >::iterator it =
      constructed.find(e);
    if (it!=constructed.end()) {
      if (width_out)
        *width_out = it->second.second;
      return it->second.first;
    } else {
      int width;
      if (!width_out) width_out = &width;
      Z3_ast res = constructActual(e, width_out);
      constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
      return res;
    }
  }
}


/** if *width_out!=1 then result is a bitvector,
    otherwise it is a bool */
Z3_ast Z3Builder::constructActual(ref<Expr> e, int *width_out) {
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
    Z3_ast Res = bvConst64(64, Tmp->Extract(0, 64)->getZExtValue());
    while (Tmp->getWidth() > 64) {
    	Tmp = Tmp->Extract(64, Tmp->getWidth()-64);
    	unsigned Width = std::min(64U, Tmp->getWidth());
    	Res = Z3_mk_concat(ctx, bvConst64(Width,
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
    Z3_ast cond = construct(se->cond, 0);
    Z3_ast tExpr = construct(se->trueExpr, width_out);
    Z3_ast fExpr = construct(se->falseExpr, width_out);
    return iteExpr(cond, tExpr, fExpr);
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    unsigned numKids = ce->getNumKids();
    Z3_ast res = construct(ce->getKid(numKids-1), 0);
    for (int i=numKids-2; i>=0; i--) {
      res = Z3_mk_concat(ctx, construct(ce->getKid(i), 0), res);
    }
    *width_out = ce->getWidth();
    return res;
  }

  case Expr::Extract: {
    ExtractExpr *ee = cast<ExtractExpr>(e);
    Z3_ast src = construct(ee->expr, width_out);
    *width_out = ee->getWidth();
    if (*width_out==1) {
      return bvBoolExtract(src, ee->offset);
    } else {
      return bvExtract(src, ee->offset + *width_out - 1, ee->offset);
    }
  }

    // Casting

  case Expr::ZExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    Z3_ast src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth==1) {
      return iteExpr(src, bvOne(*width_out), bvZero(*width_out));
    } else {
      return Z3_mk_concat(ctx, bvZero(*width_out-srcWidth), src);
    }
  }

  case Expr::SExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    Z3_ast src = construct(ce->src, &srcWidth);
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
    Z3_ast left = construct(ae->left, width_out);
    Z3_ast right = construct(ae->right, width_out);
    assert(*width_out!=1 && "uncanonicalized add");
    return bvPlusExpr(*width_out, left, right);
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    Z3_ast left = construct(se->left, width_out);
    Z3_ast right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized sub");
    return bvMinusExpr(*width_out, left, right);
  } 

  case Expr::Mul: {
	  MulExpr *me = cast<MulExpr>(e);
	  Z3_ast right = construct(me->right, width_out);
	  assert(*width_out!=1 && "uncanonicalized mul");

	  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(me->left))
		  if (CE->getWidth() <= 64)
			  return constructMulByConstant(right, *width_out,
					  CE->getZExtValue());

	  Z3_ast left = construct(me->left, width_out);
	  return bvMultExpr(*width_out, left, right);
  }

  case Expr::UDiv: {
	  UDivExpr *de = cast<UDivExpr>(e);
	  Z3_ast left = construct(de->left, width_out);
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

    Z3_ast right = construct(de->right, width_out);
    return bvDivExpr(*width_out, left, right);
  }

  case Expr::SDiv: {
    SDivExpr *de = cast<SDivExpr>(e);
    Z3_ast left = construct(de->left, width_out);
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
    Z3_ast right = construct(de->right, width_out);
    return sbvDivExpr(*width_out, left, right);
  }

  case Expr::URem: {
    URemExpr *de = cast<URemExpr>(e);
    Z3_ast left = construct(de->left, width_out);
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
            return Z3_mk_concat(ctx, bvZero(*width_out - bits),
                                   bvExtract(left, bits - 1, 0));
          }
        }

        // Use fast division to compute modulo without explicit division for
        // constant divisor.

        if (optimizeDivides) {
          if (*width_out == 32) { //only works for 32-bit division
            Z3_ast quotient = constructUDivByConstant( left, *width_out, (uint32_t)divisor );
            Z3_ast quot_times_divisor = constructMulByConstant( quotient, *width_out, divisor );
            Z3_ast rem = bvMinusExpr( *width_out, left, quot_times_divisor );
            return rem;
          }
        }
      }
    }
    
    Z3_ast right = construct(de->right, width_out);
    return bvModExpr(*width_out, left, right);
  }

  case Expr::SRem: {
    SRemExpr *de = cast<SRemExpr>(e);
    Z3_ast left = construct(de->left, width_out);
    Z3_ast right = construct(de->right, width_out);
    assert(*width_out!=1 && "uncanonicalized srem");

#if 0 //not faster per first benchmark
    if (optimizeDivides) {
      if (ConstantExpr *cre = de->right->asConstant()) {
	uint64_t divisor = cre->asUInt64;

	//use fast division to compute modulo without explicit division for constant divisor
      	if( *width_out == 32 ) { //only works for 32-bit division
	  Z3_ast quotient = constructSDivByConstant( left, *width_out, divisor );
	  Z3_ast quot_times_divisor = constructMulByConstant( quotient, *width_out, divisor );
	  Z3_ast rem = vc_bvMinusExpr( vc, *width_out, left, quot_times_divisor );
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
    Z3_ast expr = construct(ne->expr, width_out);
    if (*width_out==1) {
      return notExpr(expr);
    } else {
      return bvNotExpr(expr);
    }
  }    

  case Expr::And: {
    AndExpr *ae = cast<AndExpr>(e);
    Z3_ast left = construct(ae->left, width_out);
    Z3_ast right = construct(ae->right, width_out);
    if (*width_out==1) {
      return andExpr(left, right);
    } else {
      return bvAndExpr(left, right);
    }
  }

  case Expr::Or: {
    OrExpr *oe = cast<OrExpr>(e);
    Z3_ast left = construct(oe->left, width_out);
    Z3_ast right = construct(oe->right, width_out);
    if (*width_out==1) {
      return orExpr(left, right);
    } else {
      return bvOrExpr(left, right);
    }
  }

  case Expr::Xor: {
    XorExpr *xe = cast<XorExpr>(e);
    Z3_ast left = construct(xe->left, width_out);
    Z3_ast right = construct(xe->right, width_out);
    
    if (*width_out==1) {
      // XXX check for most efficient?
      return iteExpr(left,
                        Z3_ast(notExpr(right)), right);
    } else {
      return bvXorExpr(left, right);
    }
  }

  case Expr::Shl: {
    ShlExpr *se = cast<ShlExpr>(e);
    Z3_ast left = construct(se->left, width_out);
    assert(*width_out!=1 && "uncanonicalized shl");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
      return bvLeftShift(left, (unsigned) CE->getLimitedValue());
    } else {
      int shiftWidth;
      Z3_ast amount = construct(se->right, &shiftWidth);
      return bvVarLeftShift( left, amount);
    }
  }

  case Expr::LShr: {
    LShrExpr *lse = cast<LShrExpr>(e);
    Z3_ast left = construct(lse->left, width_out);
    assert(*width_out!=1 && "uncanonicalized lshr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
      return bvRightShift(left, (unsigned) CE->getLimitedValue());
    } else {
      int shiftWidth;
      Z3_ast amount = construct(lse->right, &shiftWidth);
      return bvVarRightShift( left, amount);
    }
  }

  case Expr::AShr: {
    AShrExpr *ase = cast<AShrExpr>(e);
    Z3_ast left = construct(ase->left, width_out);
    assert(*width_out!=1 && "uncanonicalized ashr");
    
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ase->right)) {
      unsigned shift = (unsigned) CE->getLimitedValue();
      Z3_ast signedBool = bvBoolExtract(left, *width_out-1);
      return constructAShrByConstant(left, shift, signedBool);
    } else {
      int shiftWidth;
      Z3_ast amount = construct(ase->right, &shiftWidth);
      return bvVarArithRightShift( left, amount);
    }
  }

    // Comparison

  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    Z3_ast left = construct(ee->left, width_out);
    Z3_ast right = construct(ee->right, width_out);
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
    Z3_ast left = construct(ue->left, width_out);
    Z3_ast right = construct(ue->right, width_out);
    assert(*width_out!=1 && "uncanonicalized ult");
    *width_out = 1;
    return bvLtExpr(left, right);
  }

  case Expr::Ule: {
    UleExpr *ue = cast<UleExpr>(e);
    Z3_ast left = construct(ue->left, width_out);
    Z3_ast right = construct(ue->right, width_out);
    assert(*width_out!=1 && "uncanonicalized ule");
    *width_out = 1;
    return bvLeExpr(left, right);
  }

  case Expr::Slt: {
    SltExpr *se = cast<SltExpr>(e);
    Z3_ast left = construct(se->left, width_out);
    Z3_ast right = construct(se->right, width_out);
    assert(*width_out!=1 && "uncanonicalized slt");
    *width_out = 1;
    return sbvLtExpr(left, right);
  }

  case Expr::Sle: {
    SleExpr *se = cast<SleExpr>(e);
    Z3_ast left = construct(se->left, width_out);
    Z3_ast right = construct(se->right, width_out);
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

  case Expr::Exists: {
    ExistsExpr *xe = cast<ExistsExpr>(e);
    pushQuantificationContext(xe->variables);
    Z3_ast ret = existsExpr(construct(xe->body, width_out));
    popQuantificationContext();
    *width_out = 1;
    return ret;
  }

  default: 
    assert(0 && "unhandled Expr type");
    return getTrue();
  }
}

/***/

Z3Builder::QuantificationContext::QuantificationContext(
    Z3_context _ctx, std::set<const Array *> _existentials,
    QuantificationContext *_parent)
    : parent(_parent) {
  unsigned index = _existentials.size();
  for (std::set<const Array *>::iterator it = _existentials.begin(),
                                         itEnd = _existentials.end();
       it != itEnd; ++it) {
    --index;
    Z3_symbol symb = Z3_mk_string_symbol(_ctx, (*it)->name.c_str());
    Z3_sort sort = Z3_mk_array_sort(_ctx, Z3_mk_bv_sort(_ctx, (*it)->domain),
                                    Z3_mk_bv_sort(_ctx, (*it)->range));
    existentials[(*it)->name] = Z3_mk_bound(_ctx, index, sort);
    sorts.push_back(sort);
    symbols.push_back(symb);
  }
}

Z3Builder::QuantificationContext::~QuantificationContext() {
  existentials.clear();
  sorts.clear();
  symbols.clear();
}

Z3_ast Z3Builder::QuantificationContext::getBoundVarQuick(std::string name) {
  Z3_ast ret = existentials[name];
  if (ret)
    return ret;

  if (parent)
    return parent->getBoundVarQuick(name);

  return 0;
}

Z3_ast Z3Builder::QuantificationContext::getBoundVar(std::string name) {
  if (name.find("__shadow__") != 0)
    return 0;
  return getBoundVarQuick(name);
}

#endif /* SUPPORT_Z3 */
