//===-- ArrayExprRewriter.cpp ---------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ArrayExprRewriter.h"

#include <cassert>
#include <cstdint>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/Casting.h>
#include <set>
#include <utility>

#include "ArrayExprVisitor.h"
#include "klee/util/BitArray.h"

using namespace klee;

ref<Expr>
ExprRewriter::createOptExpr(const ref<Expr> &e, const array2idx_ty &arrays,
                            const mapIndexOptimizedExpr_ty &idx_valIdx) {
  return rewrite(e, arrays, idx_valIdx);
}

ref<Expr> ExprRewriter::rewrite(const ref<Expr> &e, const array2idx_ty &arrays,
                                const mapIndexOptimizedExpr_ty &idx_valIdx) {
  ref<Expr> notFound;
  std::vector<ref<Expr>> eqExprs;
  bool invert = false;
  for (auto &element : arrays) {
    const Array *arr = element.first;
    std::vector<ref<Expr>> indexes = element.second;

    IndexTransformationExprVisitor idxt_v(arr);
    idxt_v.visit(e);

    assert((idxt_v.getWidth() % element.first->range == 0) &&
           "Read is not aligned");

    Expr::Width width = idxt_v.getWidth() / element.first->range;
    if (!idxt_v.getMul().isNull()) {
      // If we have a MulExpr in the index, we can optimize our search by
      // skipping all those indexes that are not multiple of such value.
      // In fact, they will be rejected by the MulExpr interpreter since it
      // will not find any integer solution
      auto e = idxt_v.getMul();
      auto ce = dyn_cast<ConstantExpr>(e);
      assert(ce && "Not a constant expression");

      llvm::APInt val = ce->getAPValue();
      uint64_t mulVal = val.getZExtValue();
      // So far we try to limit this optimization, but we may try some more
      // aggressive conditions (i.e. mulVal > width)
      if (width == 1 && mulVal > 1)
        width = mulVal;
    }

    for (std::vector<ref<Expr>>::const_iterator index_it = indexes.begin();
         index_it != indexes.end(); ++index_it) {
      if (idx_valIdx.find((*index_it)) == idx_valIdx.end()) {
        continue;
      }
      auto opt_indexes = idx_valIdx.at((*index_it));
      if (opt_indexes.empty()) {
        // We continue with other solutions
        continue;
      } else if (opt_indexes.size() == 1) {
        // We treat this case as a special one, and we create an EqExpr (e.g.
        // k==i)
        eqExprs.push_back(createEqExpr((*index_it), opt_indexes[0]));
      } else {
        Expr::Width idxWidth = (*index_it).get()->getWidth();
        unsigned set = 0;
        BitArray ba(arr->size / width);
        for (auto &vals : opt_indexes) {
          auto ce = dyn_cast<ConstantExpr>(vals);
          llvm::APInt v = ce->getAPValue();
          ba.set(v.getZExtValue() / width);
          set++;
        }
        if (set > 0 && set < arr->size / width)
          invert =
              ((float)set / (float)(arr->size / width)) > 0.5 ? true : false;
        int start = -1;
        for (unsigned i = 0; i < arr->size / width; ++i) {
          if ((!invert && ba.get(i)) || (invert && !ba.get(i))) {
            if (start < 0)
              start = i;
          } else {
            if (start >= 0) {
              if (i - start == 1) {
                eqExprs.push_back(createEqExpr(
                    (*index_it),
                    ConstantExpr::create(start * width, idxWidth)));
              } else {
                // create range expr
                ref<Expr> s = ConstantExpr::create(start * width, idxWidth);
                ref<Expr> e = ConstantExpr::create((i - 1) * width, idxWidth);
                eqExprs.push_back(createRangeExpr((*index_it), s, e));
              }
              start = -1;
            }
          }
        }
        if (start >= 0) {
          if ((arr->size / width) - start == 1) {
            eqExprs.push_back(createEqExpr(
                (*index_it), ConstantExpr::create(start * width, idxWidth)));
          } else {
            // create range expr
            ref<Expr> s = ConstantExpr::create(start * width, idxWidth);
            ref<Expr> e = ConstantExpr::create(
                ((arr->size / width) - 1) * width, idxWidth);
            eqExprs.push_back(createRangeExpr((*index_it), s, e));
          }
        }
      }
    }
  }
  if (eqExprs.empty()) {
    return notFound;
  } else if (eqExprs.size() == 1) {
    if (isa<AndExpr>(eqExprs[0])) {
      return EqExpr::alloc(
          ConstantExpr::alloc(invert ? 0 : 1, (eqExprs[0])->getWidth()),
          eqExprs[0]);
    }
    return invert ? NotExpr::alloc(eqExprs[0]) : eqExprs[0];
  } else {
    // We have found at least 2 indexes, we combine them using an OrExpr (e.g.
    // k==i|k==j)
    ref<Expr> orExpr = concatenateOrExpr(eqExprs.begin(), eqExprs.end());
    // Create Eq expression for true branch
    return EqExpr::alloc(
        ConstantExpr::alloc(invert ? 0 : 1, (orExpr)->getWidth()), orExpr);
  }
}

ref<Expr> ExprRewriter::concatenateOrExpr(
    const std::vector<ref<Expr>>::const_iterator begin,
    const std::vector<ref<Expr>>::const_iterator end) {
  if (begin + 2 == end) {
    return OrExpr::alloc(ZExtExpr::alloc((*begin), Expr::Int32),
                         ZExtExpr::alloc((*(begin + 1)), Expr::Int32));
  } else {
    return OrExpr::alloc(ZExtExpr::alloc((*begin), Expr::Int32),
                         concatenateOrExpr(begin + 1, end));
  }
}

ref<Expr> ExprRewriter::createEqExpr(const ref<Expr> &index,
                                     const ref<Expr> &valIndex) {
  return EqExpr::alloc(valIndex, index);
}

ref<Expr> ExprRewriter::createRangeExpr(const ref<Expr> &index,
                                        const ref<Expr> &valStart,
                                        const ref<Expr> &valEnd) {
  return AndExpr::alloc(UleExpr::alloc(valStart, index),
                        UleExpr::alloc(index, valEnd));
}
