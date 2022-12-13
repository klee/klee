//===-- Z3CoreBuilder.cpp ------------------------------------------*- C++
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

#include "Z3Builder.h"
#include "Z3CoreBuilder.h"
#include "klee/ADT/Bits.h"
#include "klee/Expr/Expr.h"
#include "klee/Solver/SolverStats.h"

#include "llvm/Support/CommandLine.h"

using namespace klee;

namespace klee {
Z3CoreBuilder::Z3CoreBuilder(bool autoClearConstructCache,
                             const char *z3LogInteractionFileArg)
    : Z3Builder(autoClearConstructCache, z3LogInteractionFileArg) {}

Z3CoreBuilder::~Z3CoreBuilder() {}

Z3ASTHandle Z3CoreBuilder::bvExtract(Z3ASTHandle expr, unsigned top,
                                     unsigned bottom) {
  return Z3ASTHandle(Z3_mk_extract(ctx, top, bottom, expr), ctx);
}

Z3ASTHandle Z3CoreBuilder::eqExpr(Z3ASTHandle a, Z3ASTHandle b) {
  return Z3ASTHandle(Z3_mk_eq(ctx, a, b), ctx);
}

// logical right shift
Z3ASTHandle Z3CoreBuilder::bvRightShift(Z3ASTHandle expr, unsigned shift) {
  unsigned width = getBVLength(expr);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return Z3ASTHandle(
        Z3_mk_concat(ctx, bvZero(shift), bvExtract(expr, width - 1, shift)),
        ctx);
  }
}

// logical left shift
Z3ASTHandle Z3CoreBuilder::bvLeftShift(Z3ASTHandle expr, unsigned shift) {
  unsigned width = getBVLength(expr);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return Z3ASTHandle(
        Z3_mk_concat(ctx, bvExtract(expr, width - shift - 1, 0), bvZero(shift)),
        ctx);
  }
}

// left shift by a variable amount on an expression of the specified width
Z3ASTHandle Z3CoreBuilder::bvVarLeftShift(Z3ASTHandle expr, Z3ASTHandle shift) {
  unsigned width = getBVLength(expr);
  Z3ASTHandle res = bvZero(width);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  for (int i = width - 1; i >= 0; i--) {
    res =
        iteExpr(eqExpr(shift, bvConst32(width, i)), bvLeftShift(expr, i), res);
  }

  // If overshifting, shift to zero
  Z3ASTHandle ex = bvLtExpr(shift, bvConst32(getBVLength(shift), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// logical right shift by a variable amount on an expression of the specified
// width
Z3ASTHandle Z3CoreBuilder::bvVarRightShift(Z3ASTHandle expr,
                                           Z3ASTHandle shift) {
  unsigned width = getBVLength(expr);
  Z3ASTHandle res = bvZero(width);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  for (int i = width - 1; i >= 0; i--) {
    res =
        iteExpr(eqExpr(shift, bvConst32(width, i)), bvRightShift(expr, i), res);
  }

  // If overshifting, shift to zero
  Z3ASTHandle ex = bvLtExpr(shift, bvConst32(getBVLength(shift), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// arithmetic right shift by a variable amount on an expression of the specified
// width
Z3ASTHandle Z3CoreBuilder::bvVarArithRightShift(Z3ASTHandle expr,
                                                Z3ASTHandle shift) {
  unsigned width = getBVLength(expr);

  // get the sign bit to fill with
  Z3ASTHandle signedBool = bvBoolExtract(expr, width - 1);

  // start with the result if shifting by width-1
  Z3ASTHandle res = constructAShrByConstant(expr, width - 1, signedBool);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  // XXX more efficient to move the ite on the sign outside all exprs?
  // XXX more efficient to sign extend, right shift, then extract lower bits?
  for (int i = width - 2; i >= 0; i--) {
    res = iteExpr(eqExpr(shift, bvConst32(width, i)),
                  constructAShrByConstant(expr, i, signedBool), res);
  }

  // If overshifting, shift to zero
  Z3ASTHandle ex = bvLtExpr(shift, bvConst32(getBVLength(shift), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

Z3ASTHandle Z3CoreBuilder::bvNotExpr(Z3ASTHandle expr) {
  return Z3ASTHandle(Z3_mk_bvnot(ctx, expr), ctx);
}

Z3ASTHandle Z3CoreBuilder::bvAndExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvand(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3CoreBuilder::bvOrExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvor(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3CoreBuilder::bvXorExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvxor(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3CoreBuilder::bvSignExtend(Z3ASTHandle src, unsigned width) {
  unsigned src_width =
      Z3_get_bv_sort_size(ctx, Z3SortHandle(Z3_get_sort(ctx, src), ctx));
  assert(src_width <= width && "attempted to extend longer data");

  return Z3ASTHandle(Z3_mk_sign_ext(ctx, width - src_width, src), ctx);
}

Z3ASTHandle Z3CoreBuilder::iteExpr(Z3ASTHandle condition, Z3ASTHandle whenTrue,
                                   Z3ASTHandle whenFalse) {
  return Z3ASTHandle(Z3_mk_ite(ctx, condition, whenTrue, whenFalse), ctx);
}

Z3ASTHandle Z3CoreBuilder::bvLtExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvult(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3CoreBuilder::bvLeExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvule(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3CoreBuilder::sbvLtExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvslt(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3CoreBuilder::sbvLeExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvsle(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3CoreBuilder::constructAShrByConstant(Z3ASTHandle expr,
                                                   unsigned shift,
                                                   Z3ASTHandle isSigned) {
  unsigned width = getBVLength(expr);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    // FIXME: Is this really the best way to interact with Z3?
    return iteExpr(isSigned,
                   Z3ASTHandle(Z3_mk_concat(ctx, bvMinusOne(shift),
                                            bvExtract(expr, width - 1, shift)),
                               ctx),
                   bvRightShift(expr, shift));
  }
}

/** if *width_out!=1 then result is a bitvector,
otherwise it is a bool */
Z3ASTHandle Z3CoreBuilder::constructActual(ref<Expr> e, int *width_out) {
  int width;
  if (!width_out)
    width_out = &width;

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
    Z3ASTHandle Res = bvConst64(64, Tmp->Extract(0, 64)->getZExtValue());
    while (Tmp->getWidth() > 64) {
      Tmp = Tmp->Extract(64, Tmp->getWidth() - 64);
      unsigned Width = std::min(64U, Tmp->getWidth());
      Res = Z3ASTHandle(
          Z3_mk_concat(ctx,
                       bvConst64(Width, Tmp->Extract(0, Width)->getZExtValue()),
                       Res),
          ctx);
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
      return Z3ASTHandle(Z3_mk_concat(ctx, bvZero(*width_out - srcWidth), src),
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

    // Arithmetic
  case Expr::Add: {
    AddExpr *ae = cast<AddExpr>(e);
    Z3ASTHandle left = construct(ae->left, width_out);
    Z3ASTHandle right = construct(ae->right, width_out);
    assert(*width_out != 1 && "uncanonicalized add");
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvadd(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    Z3ASTHandle left = construct(se->left, width_out);
    Z3ASTHandle right = construct(se->right, width_out);
    assert(*width_out != 1 && "uncanonicalized sub");
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvsub(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Mul: {
    MulExpr *me = cast<MulExpr>(e);
    Z3ASTHandle right = construct(me->right, width_out);
    assert(*width_out != 1 && "uncanonicalized mul");
    Z3ASTHandle left = construct(me->left, width_out);
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvmul(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::UDiv: {
    UDivExpr *de = cast<UDivExpr>(e);
    Z3ASTHandle left = construct(de->left, width_out);
    assert(*width_out != 1 && "uncanonicalized udiv");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();
        if (bits64::isPowerOfTwo(divisor))
          return bvRightShift(left, bits64::indexOfSingleBit(divisor));
      }
    }

    Z3ASTHandle right = construct(de->right, width_out);
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvudiv(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SDiv: {
    SDivExpr *de = cast<SDivExpr>(e);
    Z3ASTHandle left = construct(de->left, width_out);
    assert(*width_out != 1 && "uncanonicalized sdiv");
    Z3ASTHandle right = construct(de->right, width_out);
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvsdiv(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::URem: {
    URemExpr *de = cast<URemExpr>(e);
    Z3ASTHandle left = construct(de->left, width_out);
    assert(*width_out != 1 && "uncanonicalized urem");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();

        if (bits64::isPowerOfTwo(divisor)) {
          // FIXME: This should be unsigned but currently needs to be signed to
          // avoid signed-unsigned comparison in assert.
          int bits = bits64::indexOfSingleBit(divisor);

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

    Z3ASTHandle right = construct(de->right, width_out);
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvurem(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SRem: {
    SRemExpr *de = cast<SRemExpr>(e);
    Z3ASTHandle left = construct(de->left, width_out);
    Z3ASTHandle right = construct(de->right, width_out);
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
    Z3ASTHandle left = construct(ase->left, width_out);
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
} // namespace klee
#endif // ENABLE_Z3
