//===-- BitwuzlaBuilder.cpp ---------------------------------*- C++ -*-====//
//-*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"

#ifdef ENABLE_BITWUZLA

#include "BitwuzlaBuilder.h"
#include "BitwuzlaHashConfig.h"
#include "klee/ADT/Bits.h"

#include "klee/Expr/Expr.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverStats.h"
#include "klee/Support/ErrorHandling.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
DISABLE_WARNING_POP

namespace klee {

BitwuzlaArrayExprHash::~BitwuzlaArrayExprHash() {}

void BitwuzlaArrayExprHash::clear() {
  _update_node_hash.clear();
  _array_hash.clear();
}

void BitwuzlaArrayExprHash::clearUpdates() { _update_node_hash.clear(); }

BitwuzlaBuilder::BitwuzlaBuilder(bool autoClearConstructCache)
    : autoClearConstructCache(autoClearConstructCache) {}

BitwuzlaBuilder::~BitwuzlaBuilder() {
  _arr_hash.clearUpdates();
  clearSideConstraints();
}

Sort BitwuzlaBuilder::getBoolSort() {
  // FIXME: cache these
  return mk_bool_sort();
}

Sort BitwuzlaBuilder::getBvSort(unsigned width) {
  // FIXME: cache these
  return mk_bv_sort(width);
}

Sort BitwuzlaBuilder::getArraySort(Sort domainSort, Sort rangeSort) {
  // FIXME: cache these
  return mk_array_sort(domainSort, rangeSort);
}

Term BitwuzlaBuilder::buildFreshBoolConst() { return mk_const(getBoolSort()); }

Term BitwuzlaBuilder::buildArray(const char *name, unsigned indexWidth,
                                 unsigned valueWidth) {
  Sort domainSort = getBvSort(indexWidth);
  Sort rangeSort = getBvSort(valueWidth);
  Sort t = getArraySort(domainSort, rangeSort);
  return mk_const(t, std::string(name));
}

Term BitwuzlaBuilder::buildConstantArray(const char *name, unsigned indexWidth,
                                         unsigned valueWidth, unsigned value) {
  Sort domainSort = getBvSort(indexWidth);
  Sort rangeSort = getBvSort(valueWidth);
  return mk_const_array(getArraySort(domainSort, rangeSort),
                        bvConst32(valueWidth, value));
}

Term BitwuzlaBuilder::getTrue() { return mk_true(); }

Term BitwuzlaBuilder::getFalse() { return mk_false(); }

Term BitwuzlaBuilder::bvOne(unsigned width) {
  return mk_bv_one(getBvSort(width));
}
Term BitwuzlaBuilder::bvZero(unsigned width) {
  return mk_bv_zero(getBvSort(width));
}
Term BitwuzlaBuilder::bvMinusOne(unsigned width) {
  return bvZExtConst(width, (uint64_t)-1);
}
Term BitwuzlaBuilder::bvConst32(unsigned width, uint32_t value) {
  if (width < 32) {
    value &= ((1 << width) - 1);
  }
  return mk_bv_value_uint64(getBvSort(width), value);
}
Term BitwuzlaBuilder::bvConst64(unsigned width, uint64_t value) {
  if (width < 64) {
    value &= ((uint64_t(1) << width) - 1);
  }
  return mk_bv_value_uint64(getBvSort(width), value);
}
Term BitwuzlaBuilder::bvZExtConst(unsigned width, uint64_t value) {
  if (width <= 64) {
    return bvConst64(width, value);
  }
  std::vector<Term> terms = {bvConst64(64, value)};
  for (width -= 64; width > 64; width -= 64) {
    terms.push_back(bvConst64(64, 0));
  }
  terms.push_back(bvConst64(width, 0));
  return mk_term(Kind::BV_CONCAT, terms);
}

Term BitwuzlaBuilder::bvSExtConst(unsigned width, uint64_t value) {
  if (width <= 64) {
    return bvConst64(width, value);
  }

  Sort t = getBvSort(width - 64);
  if (value >> 63) {
    return mk_term(Kind::BV_CONCAT,
                   {bvMinusOne(width - 64), bvConst64(64, value)});
  }
  return mk_term(Kind::BV_CONCAT, {bvZero(width - 64), bvConst64(64, value)});
}

Term BitwuzlaBuilder::bvBoolExtract(Term expr, int bit) {
  return mk_term(Kind::EQUAL, {bvExtract(expr, bit, bit), bvOne(1)});
}

Term BitwuzlaBuilder::bvExtract(Term expr, unsigned top, unsigned bottom) {
  return mk_term(Kind::BV_EXTRACT, {castToBitVector(expr)}, {top, bottom});
}

Term BitwuzlaBuilder::eqExpr(Term a, Term b) {
  // Handle implicit bitvector/float coercision
  Sort aSort = a.sort();
  Sort bSort = b.sort();

  if (aSort.is_bool() && bSort.is_fp()) {
    // Coerce `b` to be a bitvector
    b = castToBitVector(b);
  }

  if (aSort.is_fp() && bSort.is_bool()) {
    // Coerce `a` to be a bitvector
    a = castToBitVector(a);
  }
  return mk_term(Kind::EQUAL, {a, b});
}

// logical right shift
Term BitwuzlaBuilder::bvRightShift(Term expr, unsigned shift) {
  Term exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return mk_term(Kind::BV_SHR, {exprAsBv, bvConst32(width, shift)});
  }
}

// logical left shift
Term BitwuzlaBuilder::bvLeftShift(Term expr, unsigned shift) {
  Term exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return mk_term(Kind::BV_SHL, {exprAsBv, bvConst32(width, shift)});
  }
}

// left shift by a variable amount on an expression of the specified width
Term BitwuzlaBuilder::bvVarLeftShift(Term expr, Term shift) {
  Term exprAsBv = castToBitVector(expr);
  Term shiftAsBv = castToBitVector(shift);

  unsigned width = getBVLength(exprAsBv);
  Term res = mk_term(Kind::BV_SHL, {exprAsBv, shiftAsBv});

  // If overshifting, shift to zero
  Term ex = bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// logical right shift by a variable amount on an expression of the specified
// width
Term BitwuzlaBuilder::bvVarRightShift(Term expr, Term shift) {
  Term exprAsBv = castToBitVector(expr);
  Term shiftAsBv = castToBitVector(shift);

  unsigned width = getBVLength(exprAsBv);
  Term res = mk_term(Kind::BV_SHR, {exprAsBv, shiftAsBv});

  // If overshifting, shift to zero
  Term ex = bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// arithmetic right shift by a variable amount on an expression of the specified
// width
Term BitwuzlaBuilder::bvVarArithRightShift(Term expr, Term shift) {
  Term exprAsBv = castToBitVector(expr);
  Term shiftAsBv = castToBitVector(shift);

  unsigned width = getBVLength(exprAsBv);

  Term res = mk_term(Kind::BV_ASHR, {exprAsBv, shiftAsBv});

  // If overshifting, shift to zero
  Term ex = bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

Term BitwuzlaBuilder::notExpr(Term expr) { return mk_term(Kind::NOT, {expr}); }
Term BitwuzlaBuilder::andExpr(Term lhs, Term rhs) {
  return mk_term(Kind::AND, {lhs, rhs});
}
Term BitwuzlaBuilder::orExpr(Term lhs, Term rhs) {
  return mk_term(Kind::OR, {lhs, rhs});
}
Term BitwuzlaBuilder::iffExpr(Term lhs, Term rhs) {
  return mk_term(Kind::IFF, {lhs, rhs});
}

Term BitwuzlaBuilder::bvNotExpr(Term expr) {
  return mk_term(Kind::BV_NOT, {castToBitVector(expr)});
}

Term BitwuzlaBuilder::bvAndExpr(Term lhs, Term rhs) {
  return mk_term(Kind::BV_AND, {castToBitVector(lhs), castToBitVector(rhs)});
}

Term BitwuzlaBuilder::bvOrExpr(Term lhs, Term rhs) {
  return mk_term(Kind::BV_OR, {castToBitVector(lhs), castToBitVector(rhs)});
}

Term BitwuzlaBuilder::bvXorExpr(Term lhs, Term rhs) {
  return mk_term(Kind::BV_XOR, {castToBitVector(lhs), castToBitVector(rhs)});
}

Term BitwuzlaBuilder::bvSignExtend(Term src, unsigned width) {
  Term srcAsBv = castToBitVector(src);
  unsigned src_width = srcAsBv.sort().bv_size();
  assert(src_width <= width && "attempted to extend longer data");

  if (width <= 64) {
    return mk_term(Kind::BV_SIGN_EXTEND, {srcAsBv}, {width - src_width});
  }

  Term signBit = bvBoolExtract(srcAsBv, src_width - 1);
  Term zeroExtended =
      mk_term(Kind::BV_CONCAT, {bvZero(width - src_width), src});
  Term oneExtended =
      mk_term(Kind::BV_CONCAT, {bvMinusOne(width - src_width), src});

  return mk_term(Kind::ITE, {signBit, oneExtended, zeroExtended});
}

Term BitwuzlaBuilder::writeExpr(Term array, Term index, Term value) {
  return mk_term(Kind::ARRAY_STORE, {array, index, value});
}

Term BitwuzlaBuilder::readExpr(Term array, Term index) {
  return mk_term(Kind::ARRAY_SELECT, {array, index});
}

unsigned BitwuzlaBuilder::getBVLength(Term expr) {
  if (!expr.sort().is_bv()) {
    klee_error("getBVLength() accepts only bitvector, given %s",
               expr.sort().str().c_str());
  }
  return expr.sort().bv_size();
}

Term BitwuzlaBuilder::iteExpr(Term condition, Term whenTrue, Term whenFalse) {
  // Handle implicit bitvector/float coercision
  Sort whenTrueSort = whenTrue.sort();
  Sort whenFalseSort = whenFalse.sort();

  if (whenTrueSort.is_bv() && whenFalseSort.is_fp()) {
    // Coerce `whenFalse` to be a bitvector
    whenFalse = castToBitVector(whenFalse);
  }

  if (whenTrueSort.is_fp() && whenFalseSort.is_bv()) {
    // Coerce `whenTrue` to be a bitvector
    whenTrue = castToBitVector(whenTrue);
  }
  return mk_term(Kind::ITE, {condition, whenTrue, whenFalse});
}

Term BitwuzlaBuilder::bvLtExpr(Term lhs, Term rhs) {
  return mk_term(Kind::BV_ULT, {castToBitVector(lhs), castToBitVector(rhs)});
}

Term BitwuzlaBuilder::bvLeExpr(Term lhs, Term rhs) {
  return mk_term(Kind::BV_ULE, {castToBitVector(lhs), castToBitVector(rhs)});
}

Term BitwuzlaBuilder::sbvLtExpr(Term lhs, Term rhs) {
  return mk_term(Kind::BV_SLT, {castToBitVector(lhs), castToBitVector(rhs)});
}

Term BitwuzlaBuilder::sbvLeExpr(Term lhs, Term rhs) {
  return mk_term(Kind::BV_SLE, {castToBitVector(lhs), castToBitVector(rhs)});
}

Term BitwuzlaBuilder::constructAShrByConstant(Term expr, unsigned shift,
                                              Term isSigned) {
  Term exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return exprAsBv;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    // FIXME: Is this really the best way to interact with Bitwuzla?
    Term signed_term =
        mk_term(Kind::BV_CONCAT,
                {bvMinusOne(shift), bvExtract(exprAsBv, width - 1, shift)});
    Term unsigned_term = bvRightShift(exprAsBv, shift);

    return mk_term(Kind::ITE, {isSigned, signed_term, unsigned_term});
  }
}

Term BitwuzlaBuilder::getInitialArray(const Array *root) {
  assert(root);
  Term array_expr;
  bool hashed = _arr_hash.lookupArrayExpr(root, array_expr);

  if (!hashed) {
    // Unique arrays by name, so we make sure the name is unique by
    // using the size of the array hash as a counter.
    std::string unique_id = llvm::utostr(_arr_hash._array_hash.size());
    std::string unique_name = root->getIdentifier() + unique_id;

    auto source = dyn_cast<ConstantSource>(root->source);
    auto value = (source ? source->constantValues.defaultV() : nullptr);
    if (source) {
      assert(value);
    }

    if (source && !isa<ConstantExpr>(root->size)) {
      array_expr = buildConstantArray(unique_name.c_str(), root->getDomain(),
                                      root->getRange(), value->getZExtValue(8));
    } else {
      array_expr =
          buildArray(unique_name.c_str(), root->getDomain(), root->getRange());
    }

    if (source) {
      if (auto constSize = dyn_cast<ConstantExpr>(root->size)) {
        std::vector<Term> array_assertions;
        for (size_t i = 0; i < constSize->getZExtValue(); i++) {
          auto value = source->constantValues.load(i);
          // construct(= (select i root) root->value[i]) to be asserted in
          // BitwuzlaSolver.cpp
          int width_out;
          Term array_value = construct(value, &width_out);
          assert(width_out == (int)root->getRange() &&
                 "Value doesn't match root range");
          array_assertions.push_back(
              eqExpr(readExpr(array_expr, bvConst32(root->getDomain(), i)),
                     array_value));
        }
        constant_array_assertions[root] = std::move(array_assertions);
      } else {
        for (auto &[index, value] : source->constantValues.storage()) {
          int width_out;
          Term array_value = construct(value, &width_out);
          assert(width_out == (int)root->getRange() &&
                 "Value doesn't match root range");
          array_expr = writeExpr(
              array_expr, bvConst32(root->getDomain(), index), array_value);
        }
      }
    }

    _arr_hash.hashArrayExpr(root, array_expr);
  }

  return array_expr;
}

Term BitwuzlaBuilder::getInitialRead(const Array *root, unsigned index) {
  return readExpr(getInitialArray(root), bvConst32(32, index));
}

Term BitwuzlaBuilder::getArrayForUpdate(const Array *root,
                                        const UpdateNode *un) {
  // Iterate over the update nodes, until we find a cached version of the node,
  // or no more update nodes remain
  Term un_expr;
  std::vector<const UpdateNode *> update_nodes;
  for (; un && !_arr_hash.lookupUpdateNodeExpr(un, un_expr);
       un = un->next.get()) {
    update_nodes.push_back(un);
  }
  if (!un) {
    un_expr = getInitialArray(root);
  }
  // `un_expr` now holds an expression for the array - either from cache or by
  // virtue of being the initial array expression

  // Create and cache solver expressions based on the update nodes starting from
  // the oldest
  for (const auto &un :
       llvm::make_range(update_nodes.crbegin(), update_nodes.crend())) {
    un_expr =
        writeExpr(un_expr, construct(un->index, 0), construct(un->value, 0));

    _arr_hash.hashUpdateNodeExpr(un, un_expr);
  }

  return un_expr;
}

Term BitwuzlaBuilder::construct(ref<Expr> e, int *width_out) {
  if (!BitwuzlaHashConfig::UseConstructHashBitwuzla || isa<ConstantExpr>(e)) {
    return constructActual(e, width_out);
  } else {
    ExprHashMap<std::pair<Term, unsigned>>::iterator it = constructed.find(e);
    if (it != constructed.end()) {
      if (width_out)
        *width_out = it->second.second;
      return it->second.first;
    } else {
      int width;
      if (!width_out)
        width_out = &width;
      Term res = constructActual(e, width_out);
      constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
      return res;
    }
  }
}

void BitwuzlaBuilder::FPCastWidthAssert(int *width_out, char const *msg) {
  assert(&(ConstantExpr::widthToFloatSemantics(*width_out)) !=
             &(llvm::APFloat::Bogus()) &&
         msg);
}

/** if *width_out!=1 then result is a bitvector,
otherwise it is a bool */
Term BitwuzlaBuilder::constructActual(ref<Expr> e, int *width_out) {

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

    Term Res;
    if (*width_out <= 32) {
      // Fast path.
      Res = bvConst32(*width_out, CE->getZExtValue(32));
    } else if (*width_out <= 64) {
      // Fast path.
      Res = bvConst64(*width_out, CE->getZExtValue());
    } else {
      llvm::SmallString<129> CEUValue;
      CE->getAPValue().toStringUnsigned(CEUValue);
      Res = mk_bv_value(mk_bv_sort(CE->getWidth()), CEUValue.str().str(), 10);
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
    Term cond = construct(se->cond, 0);
    Term tExpr = construct(se->trueExpr, width_out);
    Term fExpr = construct(se->falseExpr, width_out);
    return iteExpr(cond, tExpr, fExpr);
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    unsigned numKids = ce->getNumKids();
    std::vector<Term> term_args;
    term_args.reserve(numKids);

    for (unsigned i = 0; i < numKids; ++i) {
      term_args.push_back(construct(ce->getKid(i), 0));
    }

    *width_out = ce->getWidth();
    return mk_term(Kind::BV_CONCAT, term_args);
  }

  case Expr::Extract: {
    ExtractExpr *ee = cast<ExtractExpr>(e);
    Term src = construct(ee->expr, width_out);
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
    Term src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth == 1) {
      return iteExpr(src, bvOne(*width_out), bvZero(*width_out));
    } else {
      assert(*width_out > srcWidth && "Invalid width_out");
      return mk_term(Kind::BV_CONCAT,
                     {bvZero(*width_out - srcWidth), castToBitVector(src)});
    }
  }

  case Expr::SExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    Term src = construct(ce->src, &srcWidth);
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
    Term src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPExt width");
    assert(*width_out >= srcWidth && "Invalid FPExt");
    // Just use any arounding mode here as we are extending
    auto out_widths = getFloatSortFromBitWidth(*width_out);
    return mk_term(
        Kind::FP_TO_FP_FROM_FP,
        {getRoundingModeSort(llvm::APFloat::rmNearestTiesToEven), src},
        {out_widths.first, out_widths.second});
  }

  case Expr::FPTrunc: {
    int srcWidth;
    FPTruncExpr *ce = cast<FPTruncExpr>(e);
    Term src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPTrunc width");
    assert(*width_out <= srcWidth && "Invalid FPTrunc");

    auto out_widths = getFloatSortFromBitWidth(*width_out);
    return mk_term(
        Kind::FP_TO_FP_FROM_FP,
        {getRoundingModeSort(llvm::APFloat::rmNearestTiesToEven), src},
        {out_widths.first, out_widths.second});
  }

  case Expr::FPToUI: {
    int srcWidth;
    FPToUIExpr *ce = cast<FPToUIExpr>(e);
    Term src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPToUI width");
    return mk_term(Kind::FP_TO_UBV,
                   {getRoundingModeSort(ce->roundingMode), src},
                   {ce->getWidth()});
  }

  case Expr::FPToSI: {
    int srcWidth;
    FPToSIExpr *ce = cast<FPToSIExpr>(e);
    Term src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPToSI width");
    return mk_term(Kind::FP_TO_SBV,
                   {getRoundingModeSort(ce->roundingMode), src},
                   {ce->getWidth()});
  }

  case Expr::UIToFP: {
    int srcWidth;
    UIToFPExpr *ce = cast<UIToFPExpr>(e);
    Term src = castToBitVector(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid UIToFP width");

    auto out_widths = getFloatSortFromBitWidth(*width_out);
    return mk_term(Kind::FP_TO_FP_FROM_UBV,
                   {getRoundingModeSort(ce->roundingMode), src},
                   {out_widths.first, out_widths.second});
  }

  case Expr::SIToFP: {
    int srcWidth;
    SIToFPExpr *ce = cast<SIToFPExpr>(e);
    Term src = castToBitVector(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid SIToFP width");

    auto out_widths = getFloatSortFromBitWidth(*width_out);
    return mk_term(Kind::FP_TO_FP_FROM_SBV,
                   {getRoundingModeSort(ce->roundingMode), src},
                   {out_widths.first, out_widths.second});
  }

    // Arithmetic
  case Expr::Add: {
    AddExpr *ae = cast<AddExpr>(e);
    Term left = castToBitVector(construct(ae->left, width_out));
    Term right = castToBitVector(construct(ae->right, width_out));
    assert(*width_out != 1 && "uncanonicalized add");
    Term result = mk_term(Kind::BV_ADD, {left, right});
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    Term left = castToBitVector(construct(se->left, width_out));
    Term right = castToBitVector(construct(se->right, width_out));
    assert(*width_out != 1 && "uncanonicalized sub");
    Term result = mk_term(Kind::BV_SUB, {left, right});
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Mul: {
    MulExpr *me = cast<MulExpr>(e);
    Term right = castToBitVector(construct(me->right, width_out));
    assert(*width_out != 1 && "uncanonicalized mul");
    Term left = castToBitVector(construct(me->left, width_out));
    Term result = mk_term(Kind::BV_MUL, {left, right});
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::UDiv: {
    UDivExpr *de = cast<UDivExpr>(e);
    Term left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized udiv");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();
        if (bits64::isPowerOfTwo(divisor))
          return bvRightShift(left, bits64::indexOfSingleBit(divisor));
      }
    }

    Term right = castToBitVector(construct(de->right, width_out));
    Term result = mk_term(Kind::BV_UDIV, {left, right});
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SDiv: {
    SDivExpr *de = cast<SDivExpr>(e);
    Term left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized sdiv");
    Term right = castToBitVector(construct(de->right, width_out));
    Term result = mk_term(Kind::BV_SDIV, {left, right});
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::URem: {
    URemExpr *de = cast<URemExpr>(e);
    Term left = castToBitVector(construct(de->left, width_out));
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
            return mk_term(Kind::BV_CONCAT, {bvZero(*width_out - bits),
                                             bvExtract(left, bits - 1, 0)});
          }
        }
      }
    }

    Term right = castToBitVector(construct(de->right, width_out));
    Term result = mk_term(Kind::BV_UREM, {left, right});
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SRem: {
    SRemExpr *de = cast<SRemExpr>(e);
    Term left = castToBitVector(construct(de->left, width_out));
    Term right = castToBitVector(construct(de->right, width_out));
    assert(*width_out != 1 && "uncanonicalized srem");
    Term result = mk_term(Kind::BV_SREM, {left, right});
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

    // Bitwise
  case Expr::Not: {
    NotExpr *ne = cast<NotExpr>(e);
    Term expr = construct(ne->expr, width_out);
    if (*width_out == 1) {
      return notExpr(expr);
    } else {
      return bvNotExpr(expr);
    }
  }

  case Expr::And: {
    AndExpr *ae = cast<AndExpr>(e);
    Term left = construct(ae->left, width_out);
    Term right = construct(ae->right, width_out);
    if (*width_out == 1) {
      return andExpr(left, right);
    } else {
      return bvAndExpr(left, right);
    }
  }

  case Expr::Or: {
    OrExpr *oe = cast<OrExpr>(e);
    Term left = construct(oe->left, width_out);
    Term right = construct(oe->right, width_out);
    if (*width_out == 1) {
      return orExpr(left, right);
    } else {
      return bvOrExpr(left, right);
    }
  }

  case Expr::Xor: {
    XorExpr *xe = cast<XorExpr>(e);
    Term left = construct(xe->left, width_out);
    Term right = construct(xe->right, width_out);

    if (*width_out == 1) {
      // XXX check for most efficient?
      return iteExpr(left, Term(notExpr(right)), right);
    } else {
      return bvXorExpr(left, right);
    }
  }

  case Expr::Shl: {
    ShlExpr *se = cast<ShlExpr>(e);
    Term left = construct(se->left, width_out);
    assert(*width_out != 1 && "uncanonicalized shl");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
      return bvLeftShift(left, (unsigned)CE->getLimitedValue());
    } else {
      int shiftWidth;
      Term amount = construct(se->right, &shiftWidth);
      return bvVarLeftShift(left, amount);
    }
  }

  case Expr::LShr: {
    LShrExpr *lse = cast<LShrExpr>(e);
    Term left = construct(lse->left, width_out);
    assert(*width_out != 1 && "uncanonicalized lshr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
      return bvRightShift(left, (unsigned)CE->getLimitedValue());
    } else {
      int shiftWidth;
      Term amount = construct(lse->right, &shiftWidth);
      return bvVarRightShift(left, amount);
    }
  }

  case Expr::AShr: {
    AShrExpr *ase = cast<AShrExpr>(e);
    Term left = castToBitVector(construct(ase->left, width_out));
    assert(*width_out != 1 && "uncanonicalized ashr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ase->right)) {
      unsigned shift = (unsigned)CE->getLimitedValue();
      Term signedBool = bvBoolExtract(left, *width_out - 1);
      return constructAShrByConstant(left, shift, signedBool);
    } else {
      int shiftWidth;
      Term amount = construct(ase->right, &shiftWidth);
      return bvVarArithRightShift(left, amount);
    }
  }

    // Comparison

  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    Term left = construct(ee->left, width_out);
    Term right = construct(ee->right, width_out);
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
    Term left = construct(ue->left, width_out);
    Term right = construct(ue->right, width_out);
    assert(*width_out != 1 && "uncanonicalized ult");
    *width_out = 1;
    return bvLtExpr(left, right);
  }

  case Expr::Ule: {
    UleExpr *ue = cast<UleExpr>(e);
    Term left = construct(ue->left, width_out);
    Term right = construct(ue->right, width_out);
    assert(*width_out != 1 && "uncanonicalized ule");
    *width_out = 1;
    return bvLeExpr(left, right);
  }

  case Expr::Slt: {
    SltExpr *se = cast<SltExpr>(e);
    Term left = construct(se->left, width_out);
    Term right = construct(se->right, width_out);
    assert(*width_out != 1 && "uncanonicalized slt");
    *width_out = 1;
    return sbvLtExpr(left, right);
  }

  case Expr::Sle: {
    SleExpr *se = cast<SleExpr>(e);
    Term left = construct(se->left, width_out);
    Term right = construct(se->right, width_out);
    assert(*width_out != 1 && "uncanonicalized sle");
    *width_out = 1;
    return sbvLeExpr(left, right);
  }

  case Expr::FOEq: {
    FOEqExpr *fcmp = cast<FOEqExpr>(e);
    Term left = castToFloat(construct(fcmp->left, width_out));
    Term right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return mk_term(Kind::FP_EQUAL, {left, right});
  }

  case Expr::FOLt: {
    FOLtExpr *fcmp = cast<FOLtExpr>(e);
    Term left = castToFloat(construct(fcmp->left, width_out));
    Term right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return mk_term(Kind::FP_LT, {left, right});
  }

  case Expr::FOLe: {
    FOLeExpr *fcmp = cast<FOLeExpr>(e);
    Term left = castToFloat(construct(fcmp->left, width_out));
    Term right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return mk_term(Kind::FP_LEQ, {left, right});
  }

  case Expr::FOGt: {
    FOGtExpr *fcmp = cast<FOGtExpr>(e);
    Term left = castToFloat(construct(fcmp->left, width_out));
    Term right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return mk_term(Kind::FP_GT, {left, right});
  }

  case Expr::FOGe: {
    FOGeExpr *fcmp = cast<FOGeExpr>(e);
    Term left = castToFloat(construct(fcmp->left, width_out));
    Term right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return mk_term(Kind::FP_GEQ, {left, right});
  }

  case Expr::IsNaN: {
    IsNaNExpr *ine = cast<IsNaNExpr>(e);
    Term arg = castToFloat(construct(ine->expr, width_out));
    *width_out = 1;
    return mk_term(Kind::FP_IS_NAN, {arg});
  }

  case Expr::IsInfinite: {
    IsInfiniteExpr *iie = cast<IsInfiniteExpr>(e);
    Term arg = castToFloat(construct(iie->expr, width_out));
    *width_out = 1;
    return mk_term(Kind::FP_IS_INF, {arg});
  }

  case Expr::IsNormal: {
    IsNormalExpr *ine = cast<IsNormalExpr>(e);
    Term arg = castToFloat(construct(ine->expr, width_out));
    *width_out = 1;
    return mk_term(Kind::FP_IS_NORMAL, {arg});
  }

  case Expr::IsSubnormal: {
    IsSubnormalExpr *ise = cast<IsSubnormalExpr>(e);
    Term arg = castToFloat(construct(ise->expr, width_out));
    *width_out = 1;
    return mk_term(Kind::FP_IS_SUBNORMAL, {arg});
  }

  case Expr::FAdd: {
    FAddExpr *fadd = cast<FAddExpr>(e);
    Term left = castToFloat(construct(fadd->left, width_out));
    Term right = castToFloat(construct(fadd->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FAdd");
    return mk_term(Kind::FP_ADD,
                   {getRoundingModeSort(fadd->roundingMode), left, right});
  }

  case Expr::FSub: {
    FSubExpr *fsub = cast<FSubExpr>(e);
    Term left = castToFloat(construct(fsub->left, width_out));
    Term right = castToFloat(construct(fsub->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FSub");
    return mk_term(Kind::FP_SUB,
                   {getRoundingModeSort(fsub->roundingMode), left, right});
  }

  case Expr::FMul: {
    FMulExpr *fmul = cast<FMulExpr>(e);
    Term left = castToFloat(construct(fmul->left, width_out));
    Term right = castToFloat(construct(fmul->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMul");
    return mk_term(Kind::FP_MUL,
                   {getRoundingModeSort(fmul->roundingMode), left, right});
  }

  case Expr::FDiv: {
    FDivExpr *fdiv = cast<FDivExpr>(e);
    Term left = castToFloat(construct(fdiv->left, width_out));
    Term right = castToFloat(construct(fdiv->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FDiv");
    return mk_term(Kind::FP_DIV,
                   {getRoundingModeSort(fdiv->roundingMode), left, right});
  }
  case Expr::FRem: {
    FRemExpr *frem = cast<FRemExpr>(e);
    Term left = castToFloat(construct(frem->left, width_out));
    Term right = castToFloat(construct(frem->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FRem");
    return mk_term(Kind::FP_REM, {left, right});
  }

  case Expr::FMax: {
    FMaxExpr *fmax = cast<FMaxExpr>(e);
    Term left = castToFloat(construct(fmax->left, width_out));
    Term right = castToFloat(construct(fmax->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMax");
    return mk_term(Kind::FP_MAX, {left, right});
  }

  case Expr::FMin: {
    FMinExpr *fmin = cast<FMinExpr>(e);
    Term left = castToFloat(construct(fmin->left, width_out));
    Term right = castToFloat(construct(fmin->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMin");
    return mk_term(Kind::FP_MIN, {left, right});
  }

  case Expr::FSqrt: {
    FSqrtExpr *fsqrt = cast<FSqrtExpr>(e);
    Term arg = castToFloat(construct(fsqrt->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FSqrt");
    return mk_term(Kind::FP_SQRT,
                   {getRoundingModeSort(fsqrt->roundingMode), arg});
  }
  case Expr::FRint: {
    FRintExpr *frint = cast<FRintExpr>(e);
    Term arg = castToFloat(construct(frint->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FSqrt");
    return mk_term(Kind::FP_RTI,
                   {getRoundingModeSort(frint->roundingMode), arg});
  }

  case Expr::FAbs: {
    FAbsExpr *fabsExpr = cast<FAbsExpr>(e);
    Term arg = castToFloat(construct(fabsExpr->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FAbs");
    return mk_term(Kind::FP_ABS, {arg});
  }

  case Expr::FNeg: {
    FNegExpr *fnegExpr = cast<FNegExpr>(e);
    Term arg = castToFloat(construct(fnegExpr->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FNeg");
    return mk_term(Kind::FP_NEG, {arg});
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

Term BitwuzlaBuilder::fpToIEEEBV(const Term &fp) {
  if (!fp.sort().is_fp()) {
    klee_error("BitwuzlaBuilder::fpToIEEEBV accepts only floats");
  }

  Term signBit = mk_const(getBvSort(1));
  Term exponentBits = mk_const(getBvSort(fp.sort().fp_exp_size()));
  Term significandBits = mk_const(getBvSort(fp.sort().fp_sig_size() - 1));

  Term floatTerm =
      mk_term(Kind::FP_FP, {signBit, exponentBits, significandBits});
  sideConstraints.push_back(mk_term(Kind::EQUAL, {fp, floatTerm}));

  return mk_term(Kind::BV_CONCAT, {signBit, exponentBits, significandBits});
}

std::pair<unsigned, unsigned>
BitwuzlaBuilder::getFloatSortFromBitWidth(unsigned bitWidth) {
  switch (bitWidth) {
  case Expr::Int16: {
    return {5, 11};
  }
  case Expr::Int32: {
    return {8, 24};
  }
  case Expr::Int64: {
    return {11, 53};
  }
  case Expr::Fl80: {
    // Note this is an IEEE754 with a 15 bit exponent
    // and 64 bit significand. This is not the same
    // as x87 fp80 which has a different binary encoding.
    // We can use this Bitwuzla type to get the appropriate
    // amount of precision. We just have to be very
    // careful which casting between floats and bitvectors.
    //
    // Note that the number of significand bits includes the "implicit"
    // bit (which is not implicit for x87 fp80).
    return {15, 64};
  }
  case Expr::Int128: {
    return {15, 113};
  }
  default:
    assert(
        0 &&
        "bitWidth cannot converted to a IEEE-754 binary-* number by Bitwuzla");
    std::abort();
  }
}

Term BitwuzlaBuilder::castToFloat(const Term &e) {
  Sort currentSort = e.sort();
  if (currentSort.is_fp()) {
    // Already a float
    return e;
  } else if (currentSort.is_bv()) {
    unsigned bitWidth = currentSort.bv_size();
    switch (bitWidth) {
    case Expr::Int16:
    case Expr::Int32:
    case Expr::Int64:
    case Expr::Int128: {
      auto out_width = getFloatSortFromBitWidth(bitWidth);
      return mk_term(Kind::FP_TO_FP_FROM_BV, {e},
                     {out_width.first, out_width.second});
    }
    case Expr::Fl80: {
      // The bit pattern used by x87 fp80 and what we use in Bitwuzla are
      // different
      //
      // x87 fp80
      //
      // Sign Exponent Significand
      // [1]    [15]   [1] [63]
      //
      // The exponent has bias 16383 and the significand has the integer portion
      // as an explicit bit
      //
      // 79-bit IEEE-754 encoding used in Bitwuzla
      //
      // Sign Exponent [Significand]
      // [1]    [15]       [63]
      //
      // Exponent has bias 16383 (2^(15-1) -1) and the significand has
      // the integer portion as an implicit bit.
      //
      // We need to provide the mapping here and also emit a side constraint
      // to make sure the explicit bit is appropriately constrained so when
      // Bitwuzla generates a model we get the correct bit pattern back.
      //
      // This assumes Bitwuzla's IEEE semantics, x87 fp80 actually
      // has additional semantics due to the explicit bit (See 8.2.2
      // "Unsupported  Double Extended-Precision Floating-Point Encodings and
      // Pseudo-Denormals" in the Intel 64 and IA-32 Architectures Software
      // Developer's Manual) but this encoding means we can't model these
      // unsupported values in Bitwuzla.
      //
      // Note this code must kept in sync with
      // `BitwuzlaBuilder::castToBitVector()`. Which performs the inverse
      // operation here.
      //
      // TODO: Experiment with creating a constraint that transforms these
      // unsupported bit patterns into a Bitwuzla NaN to approximate the
      // behaviour from those values.

      // Note we try very hard here to avoid calling into our functions
      // here that do implicit casting so we can never recursively call
      // into this function.
      Term signBit = mk_term(Kind::BV_EXTRACT, {e}, {79, 79});
      Term exponentBits = mk_term(Kind::BV_EXTRACT, {e}, {78, 64});
      Term significandIntegerBit = mk_term(Kind::BV_EXTRACT, {e}, {63, 63});
      Term significandFractionBits = mk_term(Kind::BV_EXTRACT, {e}, {62, 0});

      Term ieeeBitPatternAsFloat = mk_term(
          Kind::FP_FP, {signBit, exponentBits, significandFractionBits});

      // Generate side constraint on the significand integer bit. It is not
      // used in `ieeeBitPatternAsFloat` so we need to constrain that bit to
      // have the correct value so that when Bitwuzla gives a model the bit
      // pattern has the right value for x87 fp80.
      //
      // If the number is a denormal or zero then the implicit integer bit
      // is zero otherwise it is one.
      Term significandIntegerBitConstrainedValue =
          getx87FP80ExplicitSignificandIntegerBit(ieeeBitPatternAsFloat);
      Term significandIntegerBitConstraint =
          mk_term(Kind::EQUAL, {significandIntegerBit,
                                significandIntegerBitConstrainedValue});

      sideConstraints.push_back(significandIntegerBitConstraint);
      return ieeeBitPatternAsFloat;
    }
    default:
      llvm_unreachable("Unhandled width when casting bitvector to float");
    }
  } else {
    llvm_unreachable("Sort cannot be cast to float");
  }
}

Term BitwuzlaBuilder::castToBitVector(const Term &e) {
  Sort currentSort = e.sort();
  if (currentSort.is_bool()) {
    return mk_term(Kind::ITE, {e, bvOne(1), bvZero(1)});
  } else if (currentSort.is_bv()) {
    // Already a bitvector
    return e;
  } else if (currentSort.is_fp()) {
    // Note this picks a single representation for NaN which means
    // `castToBitVector(castToFloat(e))` might not equal `e`.
    unsigned exponentBits = currentSort.fp_exp_size();
    unsigned significandBits =
        currentSort.fp_sig_size(); // Includes implicit bit
    unsigned floatWidth = exponentBits + significandBits;

    switch (floatWidth) {
    case Expr::Int16:
    case Expr::Int32:
    case Expr::Int64:
    case Expr::Int128:
      return fpToIEEEBV(e);
    case 79: {
      // This is Expr::Fl80 (64 bit exponent, 15 bit significand) but due to
      // the "implicit" bit actually being implicit in x87 fp80 the sum of
      // the exponent and significand bitwidth is 79 not 80.

      // Get Bitwuzla's IEEE representation
      Term ieeeBits = fpToIEEEBV(e);

      // Construct the x87 fp80 bit representation
      Term signBit = mk_term(Kind::BV_EXTRACT, {ieeeBits}, {78, 78});
      Term exponentBits = mk_term(Kind::BV_EXTRACT, {ieeeBits}, {77, 63});
      Term significandIntegerBit = getx87FP80ExplicitSignificandIntegerBit(e);
      Term significandFractionBits =
          mk_term(Kind::BV_EXTRACT, {ieeeBits}, {62, 0});
      Term x87FP80Bits = mk_term(Kind::BV_CONCAT,
                                 {signBit, exponentBits, significandIntegerBit,
                                  significandFractionBits});
      return x87FP80Bits;
    }
    default:
      llvm_unreachable("Unhandled width when casting float to bitvector");
    }
  } else {
    llvm_unreachable("Sort cannot be cast to float");
  }
}

Term BitwuzlaBuilder::getRoundingModeSort(llvm::APFloat::roundingMode rm) {
  switch (rm) {
  case llvm::APFloat::rmNearestTiesToEven:
    return mk_rm_value(RoundingMode::RNE);
  case llvm::APFloat::rmTowardPositive:
    return mk_rm_value(RoundingMode::RTP);
  case llvm::APFloat::rmTowardNegative:
    return mk_rm_value(RoundingMode::RTN);
  case llvm::APFloat::rmTowardZero:
    return mk_rm_value(RoundingMode::RTZ);
  case llvm::APFloat::rmNearestTiesToAway:
    return mk_rm_value(RoundingMode::RNA);
  default:
    llvm_unreachable("Unhandled rounding mode");
  }
}

Term BitwuzlaBuilder::getx87FP80ExplicitSignificandIntegerBit(const Term &e) {
#ifndef NDEBUG
  // Check the passed in expression is the right type.
  Sort currentSort = e.sort();
  assert(currentSort.is_fp());

  unsigned exponentBits = currentSort.fp_exp_size();
  unsigned significandBits = currentSort.fp_sig_size();
  assert(exponentBits == 15);
  assert(significandBits == 64);
#endif
  // If the number is a denormal or zero then the implicit integer bit is zero
  // otherwise it is one.  Term isDenormal =
  Term isDenormal = mk_term(Kind::FP_IS_SUBNORMAL, {e});
  Term isZero = mk_term(Kind::FP_IS_ZERO, {e});

  // FIXME: Cache these constants somewhere
  Sort oneBitBvSort = getBvSort(/*width=*/1);

  Term oneBvOne = mk_bv_value_uint64(oneBitBvSort, 1);
  Term zeroBvOne = mk_bv_value_uint64(oneBitBvSort, 0);

  Term significandIntegerBitCondition = orExpr(isDenormal, isZero);

  Term significandIntegerBitConstrainedValue =
      mk_term(Kind::ITE, {significandIntegerBitCondition, zeroBvOne, oneBvOne});

  return significandIntegerBitConstrainedValue;
}
} // namespace klee

#endif // ENABLE_BITWUZLA
