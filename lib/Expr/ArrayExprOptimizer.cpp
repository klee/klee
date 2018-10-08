#include "klee/ArrayExprOptimizer.h"

#include "klee/ArrayExprRewriter.h"
#include "klee/util/BitArray.h"

#include <iostream>

using namespace klee;

namespace klee {
llvm::cl::opt<ArrayOptimizationType> OptimizeArray(
    "optimize-array",
    llvm::cl::values(
        clEnumValN(INDEX, "index", "Index-based transformation"),
        clEnumValEnd),
    llvm::cl::init(NONE),
    llvm::cl::desc("Optimize accesses to either concrete or concrete/symbolic "
                   "arrays. (default=off)"));
};

void ExprOptimizer::optimizeExpr(const ref<Expr> &e, ref<Expr> &result,
                                 bool valueOnly) {
  unsigned hash = e->hash();
  if (cacheExprUnapplicable.find(hash) != cacheExprUnapplicable.end()) {
    return;
  }

  // Find cached expressions
  auto cached = cacheExprOptimized.find(hash);
  if (cached != cacheExprOptimized.end()) {
    result = cached->second;
    return;
  }

  // ----------------------- INDEX-BASED OPTIMIZATION -------------------------
  if (!valueOnly && OptimizeArray == INDEX) {
    array2idx_ty arrays;
    ConstantArrayExprVisitor aev(arrays);
    aev.visit(e);

    if (arrays.size() == 0 || aev.isIncompatible()) {
      // We do not optimize expressions other than those with concrete
      // arrays with a symbolic index
      // If we cannot optimize the expression, we return a failure only
      // when we are not combining the optimizations
      if (OptimizeArray == INDEX) {
        cacheExprUnapplicable.insert(hash);
        return;
      }
    } else {
      mapIndexOptimizedExpr_ty idx_valIdx;

      // Compute those indexes s.t. orig_expr =equisat= (k==i|k==j|..)
      if (computeIndexes(arrays, e, idx_valIdx)) {
        if (idx_valIdx.size() > 0) {
          // Create new expression on indexes
          result = ExprRewriter::createOptExpr(e, arrays, idx_valIdx);
        } else {
          klee_warning("OPT_I: infeasible branch!");
          result = ConstantExpr::create(0, Expr::Bool);
        }
        // Add new expression to cache
        if (result.get()) {
          klee_warning("OPT_I: successful");
          cacheExprOptimized[hash] = result;
        } else {
          klee_warning("OPT_I: unsuccessful");
        }
      } else {
        klee_warning("OPT_I: unsuccessful");
        cacheExprUnapplicable.insert(hash);
      }
    }
  }
}

bool ExprOptimizer::computeIndexes(array2idx_ty &arrays, const ref<Expr> &e,
                                   mapIndexOptimizedExpr_ty &idx_valIdx) const {
  bool success = false;
  // For each constant array found
  for (auto &element : arrays) {
    const Array *arr = element.first;

    assert(arr->isConstantArray() && "Array is not concrete");
    assert(element.second.size() == 1 && "Multiple indexes on the same array");

    IndexTransformationExprVisitor idxt_v(arr);
    idxt_v.visit(e);
    assert((idxt_v.getWidth() % arr->range == 0) && "Read is not aligned");
    Expr::Width width = idxt_v.getWidth() / arr->range;

    if (idxt_v.getMul().get()) {
      // If we have a MulExpr in the index, we can optimize our search by
      // skipping all those indexes that are not multiple of such value.
      // In fact, they will be rejected by the MulExpr interpreter since it
      // will not find any integer solution
      Expr &e = *idxt_v.getMul();
      ConstantExpr &ce = static_cast<ConstantExpr &>(e);
      uint64_t mulVal = (*ce.getAPValue().getRawData());
      // So far we try to limit this optimization, but we may try some more
      // aggressive conditions (i.e. mulVal > width)
      if (width == 1 && mulVal > 1)
        width = mulVal;
    }

    // For each concrete value 'i' stored in the array
    for (size_t aIdx = 0; aIdx < arr->constantValues.size(); aIdx += width) {
      Assignment *a = new Assignment();
      v_arr_ty objects;
      std::vector<std::vector<unsigned char> > values;

      // For each symbolic index Expr(k) found
      for (auto &index_it : element.second) {
        ref<Expr> idx = index_it;
        ref<Expr> val = ConstantExpr::alloc(aIdx, arr->getDomain());
        // We create a partial assignment on 'k' s.t. Expr(k)==i
        bool assignmentSuccess =
            AssignmentGenerator::generatePartialAssignment(idx, val, a);
        success |= assignmentSuccess;

        // If the assignment satisfies both the expression 'e' and the PC
        ref<Expr> evaluation = a->evaluate(e);
        if (assignmentSuccess && evaluation->isTrue()) {
          if (idx_valIdx.find(idx) == idx_valIdx.end()) {
            idx_valIdx.insert(std::make_pair(idx, std::vector<ref<Expr> >()));
          }
          idx_valIdx[idx]
              .push_back(ConstantExpr::alloc(aIdx, arr->getDomain()));
        }
      }
      delete a;
    }
  }
  return success;
}
