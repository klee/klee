//===-- AssignmentGenerator.cpp -------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/AssignmentGenerator.h"

#include "klee/Expr/Assignment.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/klee.h"

#include <llvm/ADT/APInt.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <stddef.h>
#include <string>
#include <utility>

using namespace klee;

bool AssignmentGenerator::generatePartialAssignment(const ref<Expr> &e,
                                                    ref<Expr> &val,
                                                    Assignment *&a) {
  return helperGenerateAssignment(e, val, a, e->getWidth(), false);
}

bool AssignmentGenerator::helperGenerateAssignment(const ref<Expr> &e,
                                                   ref<Expr> &val,
                                                   Assignment *&a,
                                                   Expr::Width width,
                                                   bool sign) {
  Expr &ep = *e.get();
  switch (ep.getKind()) {

  // ARITHMETIC
  case Expr::Add: {
    // val = val - kid
    ref<Expr> kid_val;
    ref<Expr> kid_expr;
    if (isa<ConstantExpr>(ep.getKid(0))) {
      kid_val = ep.getKid(0);
      kid_expr = ep.getKid(1);
    } else if (isa<ConstantExpr>(ep.getKid(1))) {
      kid_val = ep.getKid(1);
      kid_expr = ep.getKid(0);
    } else {
      return false;
    }
    if (!ExtractExpr::create(kid_val, kid_val.get()->getWidth() - 1,
                             1).get()->isZero()) {
      // FIXME: really bad hack to support those cases in which KLEE creates
      // Add expressions with negative values
      val = createAddExpr(kid_val, val);
    } else {
      val = createSubExpr(kid_val, val);
    }
    return helperGenerateAssignment(kid_expr, val, a, width, sign);
  }
  case Expr::Sub: {
    // val = val + kid
    ref<Expr> kid_val;
    ref<Expr> kid_expr;
    if (isa<ConstantExpr>(ep.getKid(0))) {
      kid_val = ep.getKid(0);
      kid_expr = ep.getKid(1);
    } else if (isa<ConstantExpr>(ep.getKid(1))) {
      kid_val = ep.getKid(1);
      kid_expr = ep.getKid(0);
    } else {
      return false;
    }
    val = createAddExpr(kid_val, val);
    return helperGenerateAssignment(kid_expr, val, a, width, sign);
  }
  case Expr::Mul: {
    // val = val / kid (check for sign)
    ref<Expr> kid_val;
    ref<Expr> kid_expr;
    if (isa<ConstantExpr>(ep.getKid(0))) {
      kid_val = ep.getKid(0);
      kid_expr = ep.getKid(1);
    } else if (isa<ConstantExpr>(ep.getKid(1))) {
      kid_val = ep.getKid(1);
      kid_expr = ep.getKid(0);
    } else {
      return false;
    }

    if (kid_val.get()->isZero()) {
      return false;
    } else if (!createDivRem(kid_val, val, sign)->isZero()) {
      return false;
    }
    val = createDivExpr(kid_val, val, sign);
    return helperGenerateAssignment(kid_expr, val, a, width, sign);
  }
  case Expr::UDiv:
  // val = val * kid
  // no break, falling down to case SDiv
  case Expr::SDiv: {
    // val = val * kid
    ref<Expr> kid_val;
    ref<Expr> kid_expr;
    if (isa<ConstantExpr>(ep.getKid(0))) {
      kid_val = ep.getKid(0);
      kid_expr = ep.getKid(1);
    } else if (isa<ConstantExpr>(ep.getKid(1))) {
      kid_val = ep.getKid(1);
      kid_expr = ep.getKid(0);
    } else {
      return false;
    }
    val = createMulExpr(kid_val, val);
    return helperGenerateAssignment(kid_expr, val, a, width, sign);
  }

  // LOGICAL
  case Expr::LShr: {
    if (!isa<ConstantExpr>(ep.getKid(1))) {
      return false;
    }
    ref<Expr> kid_val = ep.getKid(1);
    val = createShlExpr(val, kid_val);
    return helperGenerateAssignment(ep.getKid(0), val, a, width, sign);
  }
  case Expr::Shl: {
    if (!isa<ConstantExpr>(ep.getKid(1))) {
      return false;
    }
    ref<Expr> kid_val = ep.getKid(1);
    val = createLShrExpr(val, kid_val);
    return helperGenerateAssignment(ep.getKid(0), val, a, width, sign);
  }
  case Expr::Not: {
    return helperGenerateAssignment(ep.getKid(0), val, a, width, sign);
  }
  case Expr::And: {
    // val = val & kid
    ref<Expr> kid_val;
    ref<Expr> kid_expr;
    if (isa<ConstantExpr>(ep.getKid(0))) {
      kid_val = ep.getKid(0);
      kid_expr = ep.getKid(1);
    } else if (isa<ConstantExpr>(ep.getKid(1))) {
      kid_val = ep.getKid(1);
      kid_expr = ep.getKid(0);
    } else {
      return false;
    }
    val = createAndExpr(kid_val, val);
    return helperGenerateAssignment(kid_expr, val, a, width, sign);
  }

  // CASTING
  case Expr::ZExt: {
    val = createExtractExpr(ep.getKid(0), val);
    return helperGenerateAssignment(ep.getKid(0), val, a, width, false);
  }
  case Expr::SExt: {
    val = createExtractExpr(ep.getKid(0), val);
    return helperGenerateAssignment(ep.getKid(0), val, a, width, true);
  }

  // SPECIAL
  case Expr::Concat: {
    ReadExpr *base = hasOrderedReads(&ep);
    if (base) {
      return helperGenerateAssignment(ref<Expr>(base), val, a, ep.getWidth(),
                                      sign);
    } else {
      klee_warning("Not supported");
      ep.printKind(llvm::errs(), ep.getKind());
      return false;
    }
  }
  case Expr::Extract: {
    val = createExtendExpr(ep.getKid(0), val);
    return helperGenerateAssignment(ep.getKid(0), val, a, width, sign);
  }

  // READ
  case Expr::Read: {
    ReadExpr &re = static_cast<ReadExpr &>(ep);
    if (isa<ConstantExpr>(re.index)) {
      if (re.updates.root->isSymbolicArray()) {
        ConstantExpr &index = static_cast<ConstantExpr &>(*re.index.get());
        std::vector<unsigned char> c_value =
            getIndexedValue(getByteValue(val), index, re.updates.root->size);
        if (c_value.size() == 0) {
          return false;
        }
        if (a->bindings.find(re.updates.root) == a->bindings.end()) {
          a->bindings.insert(std::make_pair(re.updates.root, c_value));
        } else {
          return false;
        }
      }
    } else {
      return helperGenerateAssignment(re.index, val, a, width, sign);
    }
    return true;
  }
  default:
    std::string type_str;
    llvm::raw_string_ostream rso(type_str);
    ep.printKind(rso, ep.getKind());
    klee_warning("%s is not supported", rso.str().c_str());
    return false;
  }
}

bool AssignmentGenerator::isReadExprAtOffset(ref<Expr> e, const ReadExpr *base,
                                             ref<Expr> offset) {
  const ReadExpr *re = dyn_cast<ReadExpr>(e.get());
  if (!re || (re->getWidth() != Expr::Int8))
    return false;
  return SubExpr::create(re->index, base->index) == offset;
}

ReadExpr *AssignmentGenerator::hasOrderedReads(ref<Expr> e) {
  assert(e->getKind() == Expr::Concat);

  const ReadExpr *base = dyn_cast<ReadExpr>(e->getKid(0));

  // right now, all Reads are byte reads but some
  // transformations might change this
  if (!base || base->getWidth() != Expr::Int8)
    return NULL;

  // Get stride expr in proper index width.
  Expr::Width idxWidth = base->index->getWidth();
  ref<Expr> strideExpr = ConstantExpr::alloc(-1, idxWidth);
  ref<Expr> offset = ConstantExpr::create(0, idxWidth);

  e = e->getKid(1);

  // concat chains are unbalanced to the right
  while (e->getKind() == Expr::Concat) {
    offset = AddExpr::create(offset, strideExpr);
    if (!isReadExprAtOffset(e->getKid(0), base, offset))
      return NULL;
    e = e->getKid(1);
  }

  offset = AddExpr::create(offset, strideExpr);
  if (!isReadExprAtOffset(e, base, offset))
    return NULL;

  return cast<ReadExpr>(e.get());
}

ref<Expr> AssignmentGenerator::createSubExpr(const ref<Expr> &l, ref<Expr> &r) {
  return SubExpr::create(r, l);
}

ref<Expr> AssignmentGenerator::createAddExpr(const ref<Expr> &l, ref<Expr> &r) {
  return AddExpr::create(r, l);
}

ref<Expr> AssignmentGenerator::createMulExpr(const ref<Expr> &l, ref<Expr> &r) {
  return MulExpr::create(r, l);
}

ref<Expr> AssignmentGenerator::createDivRem(const ref<Expr> &l, ref<Expr> &r,
                                            bool sign) {
  if (sign)
    return SRemExpr::create(r, l);
  else
    return URemExpr::create(r, l);
}

ref<Expr> AssignmentGenerator::createDivExpr(const ref<Expr> &l, ref<Expr> &r,
                                             bool sign) {
  if (sign)
    return SDivExpr::create(r, l);
  else
    return UDivExpr::create(r, l);
}

ref<Expr> AssignmentGenerator::createShlExpr(const ref<Expr> &l, ref<Expr> &r) {
  return ShlExpr::create(r, l);
}

ref<Expr> AssignmentGenerator::createLShrExpr(const ref<Expr> &l,
                                              ref<Expr> &r) {
  return LShrExpr::create(r, l);
}

ref<Expr> AssignmentGenerator::createAndExpr(const ref<Expr> &l, ref<Expr> &r) {
  return AndExpr::create(r, l);
}

ref<Expr> AssignmentGenerator::createExtractExpr(const ref<Expr> &l,
                                                 ref<Expr> &r) {
  return ExtractExpr::create(r, 0, l.get()->getWidth());
}

ref<Expr> AssignmentGenerator::createExtendExpr(const ref<Expr> &l,
                                                ref<Expr> &r) {
  if (l.get()->getKind() == Expr::ZExt) {
    return ZExtExpr::create(r, l.get()->getWidth());
  } else {
    return SExtExpr::create(r, l.get()->getWidth());
  }
}

std::vector<unsigned char> AssignmentGenerator::getByteValue(ref<Expr> &val) {
  std::vector<unsigned char> toReturn;
  if (ConstantExpr *value = dyn_cast<ConstantExpr>(val)) {
    for (unsigned w = 0; w < val.get()->getWidth() / 8; ++w) {
      ref<ConstantExpr> byte = value->Extract(w * 8, Expr::Int8);
      unsigned char mem_val;
      byte->toMemory(&mem_val);
      toReturn.push_back(mem_val);
    }
  }
  return toReturn;
}

std::vector<unsigned char>
AssignmentGenerator::getIndexedValue(const std::vector<unsigned char> &c_val,
                                     ConstantExpr &index,
                                     const unsigned int size) {
  std::vector<unsigned char> toReturn;
  const llvm::APInt index_val = index.getAPValue();
#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
  assert(!index_val.isSignMask() && "Negative index");
#else
  assert(!index_val.isSignBit() && "Negative index");
#endif
  const uint64_t id = index_val.getZExtValue() / c_val.size();
  uint64_t arraySize = size / c_val.size();
  for (uint64_t i = 0; i < arraySize; ++i) {
    if (i == id) {
      for (unsigned arr_i = 0; arr_i < c_val.size(); ++arr_i) {
        toReturn.push_back(c_val[arr_i]);
      }
    } else {
      for (unsigned arr_i = 0; arr_i < c_val.size(); ++arr_i) {
        toReturn.push_back(0);
      }
    }
  }

  return toReturn;
}
