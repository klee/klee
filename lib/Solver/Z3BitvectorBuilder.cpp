//===-- Z3BitvectorBuilder.cpp ---------------------------------*- C++ -*-====//
//-*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"

#ifdef ENABLE_Z3

#include "Z3BitvectorBuilder.h"
#include "klee/ADT/Bits.h"

#include "klee/Expr/Expr.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverStats.h"

#include "llvm/ADT/APFloat.h"

using namespace klee;

namespace klee {
Z3BitvectorBuilder::Z3BitvectorBuilder(bool autoClearConstructCache,
                                       const char *z3LogInteractionFileArg)
    : Z3Builder(autoClearConstructCache, z3LogInteractionFileArg) {}

Z3BitvectorBuilder::~Z3BitvectorBuilder() {
  _arr_hash.clearUpdates();
  clearSideConstraints();
}

Z3ASTHandle Z3BitvectorBuilder::bvExtract(Z3ASTHandle expr, unsigned top,
                                          unsigned bottom) {
  return Z3ASTHandle(Z3_mk_extract(ctx, top, bottom, castToBitVector(expr)),
                     ctx);
}

Z3ASTHandle Z3BitvectorBuilder::eqExpr(Z3ASTHandle a, Z3ASTHandle b) {
  // Handle implicit bitvector/float coercision
  Z3SortHandle aSort = Z3SortHandle(Z3_get_sort(ctx, a), ctx);
  Z3SortHandle bSort = Z3SortHandle(Z3_get_sort(ctx, b), ctx);
  Z3_sort_kind aKind = Z3_get_sort_kind(ctx, aSort);
  Z3_sort_kind bKind = Z3_get_sort_kind(ctx, bSort);

  if (aKind == Z3_BV_SORT && bKind == Z3_FLOATING_POINT_SORT) {
    // Coerce `b` to be a bitvector
    b = castToBitVector(b);
  }

  if (aKind == Z3_FLOATING_POINT_SORT && bKind == Z3_BV_SORT) {
    // Coerce `a` to be a bitvector
    a = castToBitVector(a);
  }
  return Z3ASTHandle(Z3_mk_eq(ctx, a, b), ctx);
}

// logical right shift
Z3ASTHandle Z3BitvectorBuilder::bvRightShift(Z3ASTHandle expr, unsigned shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return Z3ASTHandle(
        Z3_mk_concat(ctx, bvZero(shift), bvExtract(exprAsBv, width - 1, shift)),
        ctx);
  }
}

// logical left shift
Z3ASTHandle Z3BitvectorBuilder::bvLeftShift(Z3ASTHandle expr, unsigned shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return Z3ASTHandle(Z3_mk_concat(ctx,
                                    bvExtract(exprAsBv, width - shift - 1, 0),
                                    bvZero(shift)),
                       ctx);
  }
}

// left shift by a variable amount on an expression of the specified width
Z3ASTHandle Z3BitvectorBuilder::bvVarLeftShift(Z3ASTHandle expr,
                                               Z3ASTHandle shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  Z3ASTHandle shiftAsBv = castToBitVector(shift);
  unsigned width = getBVLength(exprAsBv);
  Z3ASTHandle res = bvZero(width);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  for (int i = width - 1; i >= 0; i--) {
    res = iteExpr(eqExpr(shiftAsBv, bvConst32(width, i)),
                  bvLeftShift(exprAsBv, i), res);
  }

  // If overshifting, shift to zero
  Z3ASTHandle ex =
      bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// logical right shift by a variable amount on an expression of the specified
// width
Z3ASTHandle Z3BitvectorBuilder::bvVarRightShift(Z3ASTHandle expr,
                                                Z3ASTHandle shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  Z3ASTHandle shiftAsBv = castToBitVector(shift);
  unsigned width = getBVLength(exprAsBv);
  Z3ASTHandle res = bvZero(width);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  for (int i = width - 1; i >= 0; i--) {
    res = iteExpr(eqExpr(shiftAsBv, bvConst32(width, i)),
                  bvRightShift(exprAsBv, i), res);
  }

  // If overshifting, shift to zero
  Z3ASTHandle ex =
      bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// arithmetic right shift by a variable amount on an expression of the specified
// width
Z3ASTHandle Z3BitvectorBuilder::bvVarArithRightShift(Z3ASTHandle expr,
                                                     Z3ASTHandle shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  Z3ASTHandle shiftAsBv = castToBitVector(shift);
  unsigned width = getBVLength(exprAsBv);

  // get the sign bit to fill with
  Z3ASTHandle signedBool = bvBoolExtract(exprAsBv, width - 1);

  // start with the result if shifting by width-1
  Z3ASTHandle res = constructAShrByConstant(exprAsBv, width - 1, signedBool);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  // XXX more efficient to move the ite on the sign outside all exprs?
  // XXX more efficient to sign extend, right shift, then extract lower bits?
  for (int i = width - 2; i >= 0; i--) {
    res = iteExpr(eqExpr(shiftAsBv, bvConst32(width, i)),
                  constructAShrByConstant(exprAsBv, i, signedBool), res);
  }

  // If overshifting, shift to zero
  Z3ASTHandle ex =
      bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

Z3ASTHandle Z3BitvectorBuilder::bvNotExpr(Z3ASTHandle expr) {
  return Z3ASTHandle(Z3_mk_bvnot(ctx, castToBitVector(expr)), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::bvAndExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(
      Z3_mk_bvand(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::bvOrExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(
      Z3_mk_bvor(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::bvXorExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(
      Z3_mk_bvxor(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::bvSignExtend(Z3ASTHandle src, unsigned width) {
  Z3ASTHandle srcAsBv = castToBitVector(src);
  unsigned src_width =
      Z3_get_bv_sort_size(ctx, Z3SortHandle(Z3_get_sort(ctx, srcAsBv), ctx));
  assert(src_width <= width && "attempted to extend longer data");

  return Z3ASTHandle(Z3_mk_sign_ext(ctx, width - src_width, srcAsBv), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::iteExpr(Z3ASTHandle condition,
                                        Z3ASTHandle whenTrue,
                                        Z3ASTHandle whenFalse) {
  // Handle implicit bitvector/float coercision
  Z3SortHandle whenTrueSort = Z3SortHandle(Z3_get_sort(ctx, whenTrue), ctx);
  Z3SortHandle whenFalseSort = Z3SortHandle(Z3_get_sort(ctx, whenFalse), ctx);
  Z3_sort_kind whenTrueKind = Z3_get_sort_kind(ctx, whenTrueSort);
  Z3_sort_kind whenFalseKind = Z3_get_sort_kind(ctx, whenFalseSort);

  if (whenTrueKind == Z3_BV_SORT && whenFalseKind == Z3_FLOATING_POINT_SORT) {
    // Coerce `whenFalse` to be a bitvector
    whenFalse = castToBitVector(whenFalse);
  }

  if (whenTrueKind == Z3_FLOATING_POINT_SORT && whenFalseKind == Z3_BV_SORT) {
    // Coerce `whenTrue` to be a bitvector
    whenTrue = castToBitVector(whenTrue);
  }
  return Z3ASTHandle(Z3_mk_ite(ctx, condition, whenTrue, whenFalse), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::bvLtExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(
      Z3_mk_bvult(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::bvLeExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(
      Z3_mk_bvule(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::sbvLtExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(
      Z3_mk_bvslt(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::sbvLeExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(
      Z3_mk_bvsle(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3BitvectorBuilder::constructAShrByConstant(Z3ASTHandle expr,
                                                        unsigned shift,
                                                        Z3ASTHandle isSigned) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return exprAsBv;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    // FIXME: Is this really the best way to interact with Z3?
    return iteExpr(
        isSigned,
        Z3ASTHandle(Z3_mk_concat(ctx, bvMinusOne(shift),
                                 bvExtract(exprAsBv, width - 1, shift)),
                    ctx),
        bvRightShift(exprAsBv, shift));
  }
}

void Z3BitvectorBuilder::FPCastWidthAssert([[maybe_unused]] int *width_out,
                                           [[maybe_unused]] char const *msg) {
  assert(&(ConstantExpr::widthToFloatSemantics(*width_out)) !=
             &(llvm::APFloat::Bogus()) &&
         msg);
}

/** if *width_out!=1 then result is a bitvector,
otherwise it is a bool */
Z3ASTHandle Z3BitvectorBuilder::constructActual(ref<Expr> e, int *width_out) {

  int width;
  if (!width_out)
    width_out = &width;
  ++stats::queryConstructs;
  switch (e->getKind()) {
  case Expr::Pointer:
  case Expr::ConstantPointer: {
    assert(0 && "unreachable");
  }

  case Expr::Constant: {
    ConstantExpr *CE = cast<ConstantExpr>(e);
    *width_out = CE->getWidth();

    // Coerce to bool if necessary.
    if (*width_out == 1)
      return CE->isTrue() ? getTrue() : getFalse();

    Z3ASTHandle Res;
    if (*width_out <= 32) {
      // Fast path.
      Res = bvConst32(*width_out, CE->getZExtValue(32));
    } else if (*width_out <= 64) {
      // Fast path.
      Res = bvConst64(*width_out, CE->getZExtValue());
    } else {
      ref<ConstantExpr> Tmp = CE;
      Res = bvConst64(64, Tmp->Extract(0, 64)->getZExtValue());
      while (Tmp->getWidth() > 64) {
        Tmp = Tmp->Extract(64, Tmp->getWidth() - 64);
        unsigned Width = std::min(64U, Tmp->getWidth());
        Res = Z3ASTHandle(
            Z3_mk_concat(
                ctx, bvConst64(Width, Tmp->Extract(0, Width)->getZExtValue()),
                Res),
            ctx);
      }
    }
    // Coerce to float if necesary
    if (CE->isFloat()) {
      Res = castToFloat(Res);
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
    return readExpr(getArrayForUpdate(re->updates.root, re->updates.head.get()),
                    construct(re->index, 0));
  }

  case Expr::Select: {
    SelectExpr *se = cast<SelectExpr>(e);
    Z3ASTHandle cond = construct(se->cond, 0);
    Z3ASTHandle tExpr = construct(se->trueExpr, width_out);
    Z3ASTHandle fExpr = construct(se->falseExpr, width_out);
    return iteExpr(cond, tExpr, fExpr);
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    unsigned numKids = ce->getNumKids();
    Z3ASTHandle res = construct(ce->getKid(numKids - 1), 0);
    for (int i = numKids - 2; i >= 0; i--) {
      res =
          Z3ASTHandle(Z3_mk_concat(ctx, construct(ce->getKid(i), 0), res), ctx);
    }
    *width_out = ce->getWidth();
    return res;
  }

  case Expr::Extract: {
    ExtractExpr *ee = cast<ExtractExpr>(e);
    Z3ASTHandle src = construct(ee->expr, width_out);
    *width_out = ee->getWidth();
    if (*width_out == 1) {
      return bvBoolExtract(src, ee->offset);
    } else {
      return bvExtract(src, ee->offset + *width_out - 1, ee->offset);
    }
  }

    // Casting

  case Expr::ZExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    Z3ASTHandle src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth == 1) {
      return iteExpr(src, bvOne(*width_out), bvZero(*width_out));
    } else {
      assert(*width_out > srcWidth && "Invalid width_out");
      return Z3ASTHandle(Z3_mk_concat(ctx, bvZero(*width_out - srcWidth),
                                      castToBitVector(src)),
                         ctx);
    }
  }

  case Expr::SExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    Z3ASTHandle src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth == 1) {
      return iteExpr(src, bvMinusOne(*width_out), bvZero(*width_out));
    } else {
      return bvSignExtend(src, *width_out);
    }
  }

  case Expr::FPExt: {
    int srcWidth;
    FPExtExpr *ce = cast<FPExtExpr>(e);
    Z3ASTHandle src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPExt width");
    assert(*width_out >= srcWidth && "Invalid FPExt");
    // Just use any arounding mode here as we are extending
    return Z3ASTHandle(
        Z3_mk_fpa_to_fp_float(
            ctx, getRoundingModeSort(llvm::APFloat::rmNearestTiesToEven), src,
            getFloatSortFromBitWidth(*width_out)),
        ctx);
  }

  case Expr::FPTrunc: {
    int srcWidth;
    FPTruncExpr *ce = cast<FPTruncExpr>(e);
    Z3ASTHandle src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPTrunc width");
    assert(*width_out <= srcWidth && "Invalid FPTrunc");
    return Z3ASTHandle(
        Z3_mk_fpa_to_fp_float(ctx, getRoundingModeSort(ce->roundingMode), src,
                              getFloatSortFromBitWidth(*width_out)),
        ctx);
  }

  case Expr::FPToUI: {
    int srcWidth;
    FPToUIExpr *ce = cast<FPToUIExpr>(e);
    Z3ASTHandle src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPToUI width");
    return Z3ASTHandle(Z3_mk_fpa_to_ubv(ctx,
                                        getRoundingModeSort(ce->roundingMode),
                                        src, *width_out),
                       ctx);
  }

  case Expr::FPToSI: {
    int srcWidth;
    FPToSIExpr *ce = cast<FPToSIExpr>(e);
    Z3ASTHandle src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPToSI width");
    return Z3ASTHandle(Z3_mk_fpa_to_sbv(ctx,
                                        getRoundingModeSort(ce->roundingMode),
                                        src, *width_out),
                       ctx);
  }

  case Expr::UIToFP: {
    int srcWidth;
    UIToFPExpr *ce = cast<UIToFPExpr>(e);
    Z3ASTHandle src = castToBitVector(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid UIToFP width");
    return Z3ASTHandle(
        Z3_mk_fpa_to_fp_unsigned(ctx, getRoundingModeSort(ce->roundingMode),
                                 src, getFloatSortFromBitWidth(*width_out)),
        ctx);
  }

  case Expr::SIToFP: {
    int srcWidth;
    SIToFPExpr *ce = cast<SIToFPExpr>(e);
    Z3ASTHandle src = castToBitVector(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid SIToFP width");
    return Z3ASTHandle(
        Z3_mk_fpa_to_fp_signed(ctx, getRoundingModeSort(ce->roundingMode), src,
                               getFloatSortFromBitWidth(*width_out)),
        ctx);
  }

    // Arithmetic
  case Expr::Add: {
    AddExpr *ae = cast<AddExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(ae->left, width_out));
    Z3ASTHandle right = castToBitVector(construct(ae->right, width_out));
    assert(*width_out != 1 && "uncanonicalized add");
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvadd(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(se->left, width_out));
    Z3ASTHandle right = castToBitVector(construct(se->right, width_out));
    assert(*width_out != 1 && "uncanonicalized sub");
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvsub(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Mul: {
    MulExpr *me = cast<MulExpr>(e);
    Z3ASTHandle right = castToBitVector(construct(me->right, width_out));
    assert(*width_out != 1 && "uncanonicalized mul");
    Z3ASTHandle left = castToBitVector(construct(me->left, width_out));
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvmul(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::UDiv: {
    UDivExpr *de = cast<UDivExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized udiv");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();
        if (bits64::isPowerOfTwo(divisor))
          return bvRightShift(left, bits64::indexOfSingleBit(divisor));
      }
    }

    Z3ASTHandle right = castToBitVector(construct(de->right, width_out));
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvudiv(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SDiv: {
    SDivExpr *de = cast<SDivExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized sdiv");
    Z3ASTHandle right = castToBitVector(construct(de->right, width_out));
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvsdiv(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::URem: {
    URemExpr *de = cast<URemExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized urem");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();

        if (bits64::isPowerOfTwo(divisor)) {
          int bits = bits64::indexOfSingleBit(divisor);
          assert(bits >= 0 && "bit index cannot be negative");
          assert(bits64::indexOfSingleBit(divisor) < INT32_MAX);

          // special case for modding by 1 or else we bvExtract -1:0
          if (bits == 0) {
            return bvZero(*width_out);
          } else {
            assert(*width_out > bits && "invalid width_out");
            return Z3ASTHandle(Z3_mk_concat(ctx, bvZero(*width_out - bits),
                                            bvExtract(left, bits - 1, 0)),
                               ctx);
          }
        }
      }
    }

    Z3ASTHandle right = castToBitVector(construct(de->right, width_out));
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvurem(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SRem: {
    SRemExpr *de = cast<SRemExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(de->left, width_out));
    Z3ASTHandle right = castToBitVector(construct(de->right, width_out));
    assert(*width_out != 1 && "uncanonicalized srem");
    // LLVM's srem instruction says that the sign follows the dividend
    // (``left``).
    // Z3's C API says ``Z3_mk_bvsrem()`` does this so these seem to match.
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvsrem(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

    // Bitwise
  case Expr::Not: {
    NotExpr *ne = cast<NotExpr>(e);
    Z3ASTHandle expr = construct(ne->expr, width_out);
    if (*width_out == 1) {
      return notExpr(expr);
    } else {
      return bvNotExpr(expr);
    }
  }

  case Expr::And: {
    AndExpr *ae = cast<AndExpr>(e);
    Z3ASTHandle left = construct(ae->left, width_out);
    Z3ASTHandle right = construct(ae->right, width_out);
    if (*width_out == 1) {
      return andExpr(left, right);
    } else {
      return bvAndExpr(left, right);
    }
  }

  case Expr::Or: {
    OrExpr *oe = cast<OrExpr>(e);
    Z3ASTHandle left = construct(oe->left, width_out);
    Z3ASTHandle right = construct(oe->right, width_out);
    if (*width_out == 1) {
      return orExpr(left, right);
    } else {
      return bvOrExpr(left, right);
    }
  }

  case Expr::Xor: {
    XorExpr *xe = cast<XorExpr>(e);
    Z3ASTHandle left = construct(xe->left, width_out);
    Z3ASTHandle right = construct(xe->right, width_out);

    if (*width_out == 1) {
      // XXX check for most efficient?
      return iteExpr(left, Z3ASTHandle(notExpr(right)), right);
    } else {
      return bvXorExpr(left, right);
    }
  }

  case Expr::Shl: {
    ShlExpr *se = cast<ShlExpr>(e);
    Z3ASTHandle left = construct(se->left, width_out);
    assert(*width_out != 1 && "uncanonicalized shl");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
      return bvLeftShift(left, (unsigned)CE->getLimitedValue());
    } else {
      int shiftWidth;
      Z3ASTHandle amount = construct(se->right, &shiftWidth);
      return bvVarLeftShift(left, amount);
    }
  }

  case Expr::LShr: {
    LShrExpr *lse = cast<LShrExpr>(e);
    Z3ASTHandle left = construct(lse->left, width_out);
    assert(*width_out != 1 && "uncanonicalized lshr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
      return bvRightShift(left, (unsigned)CE->getLimitedValue());
    } else {
      int shiftWidth;
      Z3ASTHandle amount = construct(lse->right, &shiftWidth);
      return bvVarRightShift(left, amount);
    }
  }

  case Expr::AShr: {
    AShrExpr *ase = cast<AShrExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(ase->left, width_out));
    assert(*width_out != 1 && "uncanonicalized ashr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ase->right)) {
      unsigned shift = (unsigned)CE->getLimitedValue();
      Z3ASTHandle signedBool = bvBoolExtract(left, *width_out - 1);
      return constructAShrByConstant(left, shift, signedBool);
    } else {
      int shiftWidth;
      Z3ASTHandle amount = construct(ase->right, &shiftWidth);
      return bvVarArithRightShift(left, amount);
    }
  }

    // Comparison

  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    Z3ASTHandle left = construct(ee->left, width_out);
    Z3ASTHandle right = construct(ee->right, width_out);
    if (*width_out == 1) {
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
    Z3ASTHandle left = construct(ue->left, width_out);
    Z3ASTHandle right = construct(ue->right, width_out);
    assert(*width_out != 1 && "uncanonicalized ult");
    *width_out = 1;
    return bvLtExpr(left, right);
  }

  case Expr::Ule: {
    UleExpr *ue = cast<UleExpr>(e);
    Z3ASTHandle left = construct(ue->left, width_out);
    Z3ASTHandle right = construct(ue->right, width_out);
    assert(*width_out != 1 && "uncanonicalized ule");
    *width_out = 1;
    return bvLeExpr(left, right);
  }

  case Expr::Slt: {
    SltExpr *se = cast<SltExpr>(e);
    Z3ASTHandle left = construct(se->left, width_out);
    Z3ASTHandle right = construct(se->right, width_out);
    assert(*width_out != 1 && "uncanonicalized slt");
    *width_out = 1;
    return sbvLtExpr(left, right);
  }

  case Expr::Sle: {
    SleExpr *se = cast<SleExpr>(e);
    Z3ASTHandle left = construct(se->left, width_out);
    Z3ASTHandle right = construct(se->right, width_out);
    assert(*width_out != 1 && "uncanonicalized sle");
    *width_out = 1;
    return sbvLeExpr(left, right);
  }

  case Expr::FOEq: {
    FOEqExpr *fcmp = cast<FOEqExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_eq(ctx, left, right), ctx);
  }

  case Expr::FOLt: {
    FOLtExpr *fcmp = cast<FOLtExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_lt(ctx, left, right), ctx);
  }

  case Expr::FOLe: {
    FOLeExpr *fcmp = cast<FOLeExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_leq(ctx, left, right), ctx);
  }

  case Expr::FOGt: {
    FOGtExpr *fcmp = cast<FOGtExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_gt(ctx, left, right), ctx);
  }

  case Expr::FOGe: {
    FOGeExpr *fcmp = cast<FOGeExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_geq(ctx, left, right), ctx);
  }

  case Expr::IsNaN: {
    IsNaNExpr *ine = cast<IsNaNExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(ine->expr, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_is_nan(ctx, arg), ctx);
  }

  case Expr::IsInfinite: {
    IsInfiniteExpr *iie = cast<IsInfiniteExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(iie->expr, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_is_infinite(ctx, arg), ctx);
  }

  case Expr::IsNormal: {
    IsNormalExpr *ine = cast<IsNormalExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(ine->expr, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_is_normal(ctx, arg), ctx);
  }

  case Expr::IsSubnormal: {
    IsSubnormalExpr *ise = cast<IsSubnormalExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(ise->expr, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_is_subnormal(ctx, arg), ctx);
  }

  case Expr::FAdd: {
    FAddExpr *fadd = cast<FAddExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fadd->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fadd->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FAdd");
    return Z3ASTHandle(Z3_mk_fpa_add(ctx,
                                     getRoundingModeSort(fadd->roundingMode),
                                     left, right),
                       ctx);
  }

  case Expr::FSub: {
    FSubExpr *fsub = cast<FSubExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fsub->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fsub->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FSub");
    return Z3ASTHandle(Z3_mk_fpa_sub(ctx,
                                     getRoundingModeSort(fsub->roundingMode),
                                     left, right),
                       ctx);
  }

  case Expr::FMul: {
    FMulExpr *fmul = cast<FMulExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fmul->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fmul->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMul");
    return Z3ASTHandle(Z3_mk_fpa_mul(ctx,
                                     getRoundingModeSort(fmul->roundingMode),
                                     left, right),
                       ctx);
  }

  case Expr::FDiv: {
    FDivExpr *fdiv = cast<FDivExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fdiv->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fdiv->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FDiv");
    return Z3ASTHandle(Z3_mk_fpa_div(ctx,
                                     getRoundingModeSort(fdiv->roundingMode),
                                     left, right),
                       ctx);
  }
  case Expr::FRem: {
    FRemExpr *frem = cast<FRemExpr>(e);
    Z3ASTHandle left = castToFloat(construct(frem->left, width_out));
    Z3ASTHandle right = castToFloat(construct(frem->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FRem");
    return Z3ASTHandle(Z3_mk_fpa_rem(ctx, left, right), ctx);
  }

  case Expr::FMax: {
    FMaxExpr *fmax = cast<FMaxExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fmax->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fmax->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMax");
    return Z3ASTHandle(Z3_mk_fpa_max(ctx, left, right), ctx);
  }

  case Expr::FMin: {
    FMinExpr *fmin = cast<FMinExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fmin->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fmin->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMin");
    return Z3ASTHandle(Z3_mk_fpa_min(ctx, left, right), ctx);
  }

  case Expr::FSqrt: {
    FSqrtExpr *fsqrt = cast<FSqrtExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(fsqrt->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FSqrt");
    return Z3ASTHandle(
        Z3_mk_fpa_sqrt(ctx, getRoundingModeSort(fsqrt->roundingMode), arg),
        ctx);
  }
  case Expr::FRint: {
    FRintExpr *frint = cast<FRintExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(frint->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FSqrt");
    return Z3ASTHandle(Z3_mk_fpa_round_to_integral(
                           ctx, getRoundingModeSort(frint->roundingMode), arg),
                       ctx);
  }

  case Expr::FAbs: {
    FAbsExpr *fabsExpr = cast<FAbsExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(fabsExpr->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FAbs");
    return Z3ASTHandle(Z3_mk_fpa_abs(ctx, arg), ctx);
  }

  case Expr::FNeg: {
    FNegExpr *fnegExpr = cast<FNegExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(fnegExpr->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FNeg");
    return Z3ASTHandle(Z3_mk_fpa_neg(ctx, arg), ctx);
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
    return getTrue();
  }
}

Z3SortHandle Z3BitvectorBuilder::getFloatSortFromBitWidth(unsigned bitWidth) {
  switch (bitWidth) {
  case Expr::Int16: {
    return Z3SortHandle(Z3_mk_fpa_sort_16(ctx), ctx);
  }
  case Expr::Int32: {
    return Z3SortHandle(Z3_mk_fpa_sort_32(ctx), ctx);
  }
  case Expr::Int64: {
    return Z3SortHandle(Z3_mk_fpa_sort_64(ctx), ctx);
  }
  case Expr::Fl80: {
    // Note this is an IEEE754 with a 15 bit exponent
    // and 64 bit significand. This is not the same
    // as x87 fp80 which has a different binary encoding.
    // We can use this Z3 type to get the appropriate
    // amount of precision. We just have to be very
    // careful which casting between floats and bitvectors.
    //
    // Note that the number of significand bits includes the "implicit"
    // bit (which is not implicit for x87 fp80).
    return Z3SortHandle(Z3_mk_fpa_sort(ctx, /*ebits=*/15, /*sbits=*/64), ctx);
  }
  case Expr::Int128: {
    return Z3SortHandle(Z3_mk_fpa_sort_128(ctx), ctx);
  }
  default:
    assert(0 &&
           "bitWidth cannot converted to a IEEE-754 binary-* number by Z3");
    unreachable();
  }
}

Z3ASTHandle Z3BitvectorBuilder::castToFloat(const Z3ASTHandle &e) {
  Z3SortHandle currentSort = Z3SortHandle(Z3_get_sort(ctx, e), ctx);
  Z3_sort_kind kind = Z3_get_sort_kind(ctx, currentSort);
  switch (kind) {
  case Z3_FLOATING_POINT_SORT:
    // Already a float
    return e;
  case Z3_BV_SORT: {
    unsigned bitWidth = Z3_get_bv_sort_size(ctx, currentSort);
    switch (bitWidth) {
    case Expr::Int16:
    case Expr::Int32:
    case Expr::Int64:
    case Expr::Int128:
      return Z3ASTHandle(
          Z3_mk_fpa_to_fp_bv(ctx, e, getFloatSortFromBitWidth(bitWidth)), ctx);
    case Expr::Fl80: {
      // The bit pattern used by x87 fp80 and what we use in Z3 are different
      //
      // x87 fp80
      //
      // Sign Exponent Significand
      // [1]    [15]   [1] [63]
      //
      // The exponent has bias 16383 and the significand has the integer portion
      // as an explicit bit
      //
      // 79-bit IEEE-754 encoding used in Z3
      //
      // Sign Exponent [Significand]
      // [1]    [15]       [63]
      //
      // Exponent has bias 16383 (2^(15-1) -1) and the significand has
      // the integer portion as an implicit bit.
      //
      // We need to provide the mapping here and also emit a side constraint
      // to make sure the explicit bit is appropriately constrained so when
      // Z3 generates a model we get the correct bit pattern back.
      //
      // This assumes Z3's IEEE semantics, x87 fp80 actually
      // has additional semantics due to the explicit bit (See 8.2.2
      // "Unsupported  Double Extended-Precision Floating-Point Encodings and
      // Pseudo-Denormals" in the Intel 64 and IA-32 Architectures Software
      // Developer's Manual) but this encoding means we can't model these
      // unsupported values in Z3.
      //
      // Note this code must kept in sync with
      // `Z3BitvectorBuilder::castToBitVector()`. Which performs the inverse
      // operation here.
      //
      // TODO: Experiment with creating a constraint that transforms these
      // unsupported bit patterns into a Z3 NaN to approximate the behaviour
      // from those values.

      // Note we try very hard here to avoid calling into our functions
      // here that do implicit casting so we can never recursively call
      // into this function.
      Z3ASTHandle signBit =
          Z3ASTHandle(Z3_mk_extract(ctx, /*high=*/79, /*low=*/79, e), ctx);
      Z3ASTHandle exponentBits =
          Z3ASTHandle(Z3_mk_extract(ctx, /*high=*/78, /*low=*/64, e), ctx);
      Z3ASTHandle significandIntegerBit =
          Z3ASTHandle(Z3_mk_extract(ctx, /*high=*/63, /*low=*/63, e), ctx);
      Z3ASTHandle significandFractionBits =
          Z3ASTHandle(Z3_mk_extract(ctx, /*high=*/62, /*low=*/0, e), ctx);

      Z3ASTHandle ieeeBitPattern =
          Z3ASTHandle(Z3_mk_concat(ctx, signBit, exponentBits), ctx);
      ieeeBitPattern = Z3ASTHandle(
          Z3_mk_concat(ctx, ieeeBitPattern, significandFractionBits), ctx);
#ifndef NDEBUG
      Z3SortHandle ieeeBitPatternSort =
          Z3SortHandle(Z3_get_sort(ctx, ieeeBitPattern), ctx);
      assert(Z3_get_sort_kind(ctx, ieeeBitPatternSort) == Z3_BV_SORT);
      assert(Z3_get_bv_sort_size(ctx, ieeeBitPatternSort) == 79);
#endif

      Z3ASTHandle ieeeBitPatternAsFloat =
          Z3ASTHandle(Z3_mk_fpa_to_fp_bv(ctx, ieeeBitPattern,
                                         getFloatSortFromBitWidth(bitWidth)),
                      ctx);

      // Generate side constraint on the significand integer bit. It is not
      // used in `ieeeBitPatternAsFloat` so we need to constrain that bit to
      // have the correct value so that when Z3 gives a model the bit pattern
      // has the right value for x87 fp80.
      //
      // If the number is a denormal or zero then the implicit integer bit is
      // zero otherwise it is one.
      Z3ASTHandle significandIntegerBitConstrainedValue =
          getx87FP80ExplicitSignificandIntegerBit(ieeeBitPatternAsFloat);
      Z3ASTHandle significandIntegerBitConstraint =
          Z3ASTHandle(Z3_mk_eq(ctx, significandIntegerBit,
                               significandIntegerBitConstrainedValue),
                      ctx);
#ifndef NDEBUG
      // We will generate side constraints for constants too so we
      // need to be really careful not to generate false! Check this
      // by trying to simplify the side constraint.
      Z3ASTHandle simplifiedSignificandIntegerBitConstraint =
          Z3ASTHandle(Z3_simplify(ctx, significandIntegerBitConstraint), ctx);
      Z3_lbool simplifiedValue =
          Z3_get_bool_value(ctx, simplifiedSignificandIntegerBitConstraint);
      if (simplifiedValue == Z3_L_FALSE) {
        llvm::errs() << "Generated side constraint:\n";
        significandIntegerBitConstraint.dump();
        llvm::errs() << "\nSimplifies to false.";
        abort();
      }
#endif
      sideConstraints.push_back(significandIntegerBitConstraint);
      return ieeeBitPatternAsFloat;
    }
    default:
      llvm_unreachable("Unhandled width when casting bitvector to float");
    }
  }
  default:
    llvm_unreachable("Sort cannot be cast to float");
  }
}

Z3ASTHandle Z3BitvectorBuilder::castToBitVector(const Z3ASTHandle &e) {
  Z3SortHandle currentSort = Z3SortHandle(Z3_get_sort(ctx, e), ctx);
  Z3_sort_kind kind = Z3_get_sort_kind(ctx, currentSort);
  switch (kind) {
  case Z3_BOOL_SORT:
    return Z3ASTHandle(Z3_mk_ite(ctx, e, bvOne(1), bvZero(1)), ctx);
  case Z3_BV_SORT:
    // Already a bitvector
    return e;
  case Z3_FLOATING_POINT_SORT: {
    // Note this picks a single representation for NaN which means
    // `castToBitVector(castToFloat(e))` might not equal `e`.
    unsigned exponentBits = Z3_fpa_get_ebits(ctx, currentSort);
    unsigned significandBits =
        Z3_fpa_get_sbits(ctx, currentSort); // Includes implicit bit
    unsigned floatWidth = exponentBits + significandBits;
    switch (floatWidth) {
    case Expr::Int16:
    case Expr::Int32:
    case Expr::Int64:
    case Expr::Int128:
      return Z3ASTHandle(Z3_mk_fpa_to_ieee_bv(ctx, e), ctx);
    case 79: {
      // This is Expr::Fl80 (64 bit exponent, 15 bit significand) but due to
      // the "implicit" bit actually being implicit in x87 fp80 the sum of
      // the exponent and significand bitwidth is 79 not 80.

      // Get Z3's IEEE representation
      Z3ASTHandle ieeeBits = Z3ASTHandle(Z3_mk_fpa_to_ieee_bv(ctx, e), ctx);

      // Construct the x87 fp80 bit representation
      Z3ASTHandle signBit = Z3ASTHandle(
          Z3_mk_extract(ctx, /*high=*/78, /*low=*/78, ieeeBits), ctx);
      Z3ASTHandle exponentBits = Z3ASTHandle(
          Z3_mk_extract(ctx, /*high=*/77, /*low=*/63, ieeeBits), ctx);
      Z3ASTHandle significandIntegerBit =
          getx87FP80ExplicitSignificandIntegerBit(e);
      Z3ASTHandle significandFractionBits = Z3ASTHandle(
          Z3_mk_extract(ctx, /*high=*/62, /*low=*/0, ieeeBits), ctx);

      Z3ASTHandle x87FP80Bits =
          Z3ASTHandle(Z3_mk_concat(ctx, signBit, exponentBits), ctx);
      x87FP80Bits = Z3ASTHandle(
          Z3_mk_concat(ctx, x87FP80Bits, significandIntegerBit), ctx);
      x87FP80Bits = Z3ASTHandle(
          Z3_mk_concat(ctx, x87FP80Bits, significandFractionBits), ctx);
#ifndef NDEBUG
      Z3SortHandle x87FP80BitsSort =
          Z3SortHandle(Z3_get_sort(ctx, x87FP80Bits), ctx);
      assert(Z3_get_sort_kind(ctx, x87FP80BitsSort) == Z3_BV_SORT);
      assert(Z3_get_bv_sort_size(ctx, x87FP80BitsSort) == 80);
#endif
      return x87FP80Bits;
    }
    default:
      llvm_unreachable("Unhandled width when casting float to bitvector");
    }
  }
  default:
    llvm_unreachable("Sort cannot be cast to float");
  }
}

Z3ASTHandle
Z3BitvectorBuilder::getRoundingModeSort(llvm::APFloat::roundingMode rm) {
  switch (rm) {
  case llvm::APFloat::rmNearestTiesToEven:
    return Z3ASTHandle(Z3_mk_fpa_round_nearest_ties_to_even(ctx), ctx);
  case llvm::APFloat::rmTowardPositive:
    return Z3ASTHandle(Z3_mk_fpa_round_toward_positive(ctx), ctx);
  case llvm::APFloat::rmTowardNegative:
    return Z3ASTHandle(Z3_mk_fpa_round_toward_negative(ctx), ctx);
  case llvm::APFloat::rmTowardZero:
    return Z3ASTHandle(Z3_mk_fpa_round_toward_zero(ctx), ctx);
  case llvm::APFloat::rmNearestTiesToAway:
    return Z3ASTHandle(Z3_mk_fpa_round_nearest_ties_to_away(ctx), ctx);
  default:
    llvm_unreachable("Unhandled rounding mode");
  }
}

Z3ASTHandle Z3BitvectorBuilder::getx87FP80ExplicitSignificandIntegerBit(
    const Z3ASTHandle &e) {
#ifndef NDEBUG
  // Check the passed in expression is the right type.
  Z3SortHandle currentSort = Z3SortHandle(Z3_get_sort(ctx, e), ctx);
  assert(Z3_get_sort_kind(ctx, currentSort) == Z3_FLOATING_POINT_SORT);
  unsigned exponentBits = Z3_fpa_get_ebits(ctx, currentSort);
  unsigned significandBits = Z3_fpa_get_sbits(ctx, currentSort);
  assert(exponentBits == 15);
  assert(significandBits == 64);
#endif
  // If the number is a denormal or zero then the implicit integer bit is zero
  // otherwise it is one.  Z3ASTHandle isDenormal =
  Z3ASTHandle isDenormal = Z3ASTHandle(Z3_mk_fpa_is_subnormal(ctx, e), ctx);
  Z3ASTHandle isZero = Z3ASTHandle(Z3_mk_fpa_is_zero(ctx, e), ctx);

  // FIXME: Cache these constants somewhere
  Z3SortHandle oneBitBvSort = getBvSort(/*width=*/1);
#ifndef NDEBUG
  assert(Z3_get_sort_kind(ctx, oneBitBvSort) == Z3_BV_SORT);
  assert(Z3_get_bv_sort_size(ctx, oneBitBvSort) == 1);
#endif
  Z3ASTHandle oneBvOne =
      Z3ASTHandle(Z3_mk_unsigned_int64(ctx, 1, oneBitBvSort), ctx);
  Z3ASTHandle zeroBvOne =
      Z3ASTHandle(Z3_mk_unsigned_int64(ctx, 0, oneBitBvSort), ctx);
  Z3ASTHandle significandIntegerBitCondition = orExpr(isDenormal, isZero);
  Z3ASTHandle significandIntegerBitConstrainedValue = Z3ASTHandle(
      Z3_mk_ite(ctx, significandIntegerBitCondition, zeroBvOne, oneBvOne), ctx);
  return significandIntegerBitConstrainedValue;
}
} // namespace klee
#endif // ENABLE_Z3
