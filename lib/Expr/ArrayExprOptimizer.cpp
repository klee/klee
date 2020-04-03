//===-- ArrayExprOptimizer.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/ArrayExprOptimizer.h"

#include "klee/ADT/BitArray.h"
#include "klee/Config/Version.h"
#include "klee/Expr/ArrayExprRewriter.h"
#include "klee/Expr/ArrayExprVisitor.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/AssignmentGenerator.h"
#include "klee/Expr/ExprBuilder.h"
#include "klee/Support/OptionCategories.h"
#include "klee/Support/ErrorHandling.h"

#include <llvm/ADT/APInt.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <set>

using namespace klee;

namespace klee {
llvm::cl::opt<ArrayOptimizationType> OptimizeArray(
    "optimize-array",
    llvm::cl::values(clEnumValN(ALL, "all",
                                "Combining index and value transformations"),
                     clEnumValN(INDEX, "index", "Index-based transformation"),
                     clEnumValN(VALUE, "value",
                                "Value-based transformation at branch (both "
                                "concrete and concrete/symbolic)")
                         KLEE_LLVM_CL_VAL_END),
    llvm::cl::init(NONE),
    llvm::cl::desc("Optimize accesses to either concrete or concrete/symbolic "
                   "arrays. (default=false)"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<double> ArrayValueRatio(
    "array-value-ratio",
    llvm::cl::desc("Maximum ratio of unique values to array size for which the "
                   "value-based transformations are applied."),
    llvm::cl::init(1.0), llvm::cl::value_desc("Unique Values / Array Size"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<double> ArrayValueSymbRatio(
    "array-value-symb-ratio",
    llvm::cl::desc("Maximum ratio of symbolic values to array size for which "
                   "the mixed value-based transformations are applied."),
    llvm::cl::init(1.0), llvm::cl::value_desc("Symbolic Values / Array Size"),
    llvm::cl::cat(klee::SolvingCat));
}; // namespace klee

ref<Expr> extendRead(const UpdateList &ul, const ref<Expr> index,
                     Expr::Width w) {
  switch (w) {
  default:
    assert(0 && "invalid width");
  case Expr::Int8:
    return ReadExpr::alloc(ul, index);
  case Expr::Int16:
    return ConcatExpr::create(
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(1, Expr::Int32), index)),
        ReadExpr::alloc(ul, index));
  case Expr::Int32:
    return ConcatExpr::create4(
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(3, Expr::Int32), index)),
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(2, Expr::Int32), index)),
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(1, Expr::Int32), index)),
        ReadExpr::alloc(ul, index));
  case Expr::Int64:
    return ConcatExpr::create8(
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(7, Expr::Int32), index)),
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(6, Expr::Int32), index)),
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(5, Expr::Int32), index)),
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(4, Expr::Int32), index)),
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(3, Expr::Int32), index)),
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(2, Expr::Int32), index)),
        ReadExpr::alloc(
            ul, AddExpr::create(ConstantExpr::create(1, Expr::Int32), index)),
        ReadExpr::alloc(ul, index));
  }
}

ref<Expr> ExprOptimizer::optimizeExpr(const ref<Expr> &e, bool valueOnly) {
  // Nothing to optimise for constant expressions
  if (isa<ConstantExpr>(e))
    return e;

  // If no is optimization enabled, return early avoid cache lookup
  if (OptimizeArray == NONE)
    return e;

  if (cacheExprUnapplicable.count(e) > 0)
    return e;

  // Find cached expressions
  auto cached = cacheExprOptimized.find(e);
  if (cached != cacheExprOptimized.end())
    return cached->second;

  ref<Expr> result;
  // ----------------------- INDEX-BASED OPTIMIZATION -------------------------
  if (!valueOnly && (OptimizeArray == ALL || OptimizeArray == INDEX)) {
    array2idx_ty arrays;
    ConstantArrayExprVisitor aev(arrays);
    aev.visit(e);

    if (arrays.empty() || aev.isIncompatible()) {
      // We do not optimize expressions other than those with concrete
      // arrays with a symbolic index
      // If we cannot optimize the expression, we return a failure only
      // when we are not combining the optimizations
      if (OptimizeArray == INDEX) {
        cacheExprUnapplicable.insert(e);
        return e;
      }
    } else {
      mapIndexOptimizedExpr_ty idx_valIdx;

      // Compute those indexes s.t. orig_expr =equisat= (k==i|k==j|..)
      if (computeIndexes(arrays, e, idx_valIdx)) {
        if (!idx_valIdx.empty()) {
          // Create new expression on indexes
          result = ExprRewriter::createOptExpr(e, arrays, idx_valIdx);
        } else {
          klee_warning("OPT_I: infeasible branch!");
          result = ConstantExpr::create(0, Expr::Bool);
        }
        // Add new expression to cache
        if (result.get()) {
          klee_warning("OPT_I: successful");
          cacheExprOptimized[e] = result;
        } else {
          klee_warning("OPT_I: unsuccessful");
        }
      } else {
        klee_warning("OPT_I: unsuccessful");
        cacheExprUnapplicable.insert(e);
      }
    }
  }
  // ----------------------- VALUE-BASED OPTIMIZATION -------------------------
  if (OptimizeArray == VALUE ||
      (OptimizeArray == ALL && (!result.get() || valueOnly))) {
    std::vector<const ReadExpr *> reads;
    std::map<const ReadExpr *, std::pair<ref<Expr>, Expr::Width>> readInfo;
    ArrayReadExprVisitor are(reads, readInfo);
    are.visit(e);
    std::reverse(reads.begin(), reads.end());

    if (reads.empty() || are.isIncompatible()) {
      cacheExprUnapplicable.insert(e);
      return e;
    }

    ref<Expr> selectOpt =
        getSelectOptExpr(e, reads, readInfo, are.containsSymbolic());
    if (selectOpt.get()) {
      klee_warning("OPT_V: successful");
      result = selectOpt;
      cacheExprOptimized[e] = result;
    } else {
      klee_warning("OPT_V: unsuccessful");
      cacheExprUnapplicable.insert(e);
    }
  }
  if (result.isNull())
    return e;
  return result;
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
      auto e = idxt_v.getMul();
      auto ce = dyn_cast<ConstantExpr>(e);
      assert(ce && "Not a constant expression");
      uint64_t mulVal = (*ce->getAPValue().getRawData());
      // So far we try to limit this optimization, but we may try some more
      // aggressive conditions (i.e. mulVal > width)
      if (width == 1 && mulVal > 1)
        width = mulVal;
    }

    // For each concrete value 'i' stored in the array
    for (size_t aIdx = 0; aIdx < arr->constantValues.size(); aIdx += width) {
      auto *a = new Assignment();
      std::vector<const Array *> objects;
      std::vector<std::vector<unsigned char>> values;

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
            idx_valIdx.insert(std::make_pair(idx, std::vector<ref<Expr>>()));
          }
          idx_valIdx[idx].emplace_back(
              ConstantExpr::alloc(aIdx, arr->getDomain()));
        }
      }
      delete a;
    }
  }
  return success;
}

ref<Expr> ExprOptimizer::getSelectOptExpr(
    const ref<Expr> &e, std::vector<const ReadExpr *> &reads,
    std::map<const ReadExpr *, std::pair<ref<Expr>, Expr::Width>> &readInfo,
    bool isSymbolic) {
  ref<Expr> notFound;
  ref<Expr> toReturn;

  // Array is concrete
  if (!isSymbolic) {
    ExprHashMap<ref<Expr>> optimized;
    for (auto &read : reads) {
      auto info = readInfo[read];
      auto cached = cacheReadExprOptimized.find(const_cast<ReadExpr *>(read));
      if (cached != cacheReadExprOptimized.end()) {
        optimized.insert(std::make_pair(info.first, (*cached).second));
        continue;
      }
      Expr::Width width = read->getWidth();
      if (info.second > width) {
        width = info.second;
      }
      unsigned size = read->updates.root->getSize();
      unsigned bytesPerElement = width / 8;
      unsigned elementsInArray = size / bytesPerElement;

      // Note: we already filtered the ReadExpr, so here we can safely
      // assume that the UpdateNodes contain ConstantExpr indexes and values
      assert(read->updates.root->isConstantArray() &&
             "Expected concrete array, found symbolic array");

      // We need to read updates from lest recent to most recent, therefore
      // reverse the list
      std::vector<const UpdateNode *> us;
      us.reserve(read->updates.getSize());
      for (const UpdateNode *un = read->updates.head.get(); un;
           un = un->next.get())
        us.push_back(un);

      auto arrayConstValues = read->updates.root->constantValues;
      for (auto it = us.rbegin(); it != us.rend(); it++) {
        const UpdateNode *un = *it;
        auto ce = dyn_cast<ConstantExpr>(un->index);
        assert(ce && "Not a constant expression");
        uint64_t index = ce->getAPValue().getZExtValue();
        assert(index < arrayConstValues.size());
        auto arrayValue = dyn_cast<ConstantExpr>(un->value);
        assert(arrayValue && "Not a constant expression");
        arrayConstValues[index] = arrayValue;
      }
      std::vector<uint64_t> arrayValues;
      // Get the concrete values from the array
      for (unsigned i = 0; i < elementsInArray; i++) {
        uint64_t val = 0;
        for (unsigned j = 0; j < bytesPerElement; j++) {
          val |= (*(
                       arrayConstValues[(i * bytesPerElement) + j]
                           .get()
                           ->getAPValue()
                           .getRawData())
                  << (j * 8));
        }
        arrayValues.push_back(val);
      }

      ref<Expr> index = UDivExpr::create(
          read->index,
          ConstantExpr::create(bytesPerElement, read->index->getWidth()));

      ref<Expr> opt =
          buildConstantSelectExpr(index, arrayValues, width, elementsInArray);
      if (opt.get()) {
        cacheReadExprOptimized[const_cast<ReadExpr *>(read)] = opt;
        optimized.insert(std::make_pair(info.first, opt));
      }
    }
    ArrayValueOptReplaceVisitor replacer(optimized);
    toReturn = replacer.visit(e);
  }

  // Array is mixed concrete/symbolic
  // \pre: array is concrete && updatelist contains at least one symbolic value
  // OR
  //       array is symbolic && updatelist contains at least one concrete value
  else {
    ExprHashMap<ref<Expr>> optimized;
    for (auto &read : reads) {
      auto info = readInfo[read];
      auto cached = cacheReadExprOptimized.find(const_cast<ReadExpr *>(read));
      if (cached != cacheReadExprOptimized.end()) {
        optimized.insert(std::make_pair(info.first, (*cached).second));
        continue;
      }
      Expr::Width width = read->getWidth();
      if (info.second > width) {
        width = info.second;
      }
      unsigned size = read->updates.root->getSize();
      unsigned bytesPerElement = width / 8;
      unsigned elementsInArray = size / bytesPerElement;
      bool symbArray = read->updates.root->isSymbolicArray();

      BitArray ba(size, symbArray);
      // Note: we already filtered the ReadExpr, so here we can safely
      // assume that the UpdateNodes contain ConstantExpr indexes, but in
      // this case we *cannot* assume anything on the values
      auto arrayConstValues = read->updates.root->constantValues;
      if (arrayConstValues.size() < size) {
        // We need to "force" initialization of the values
        for (size_t i = arrayConstValues.size(); i < size; i++) {
          arrayConstValues.push_back(ConstantExpr::create(0, Expr::Int8));
        }
      }

      // We need to read updates from lest recent to most recent, therefore
      // reverse the list
      std::vector<const UpdateNode *> us;
      us.reserve(read->updates.getSize());
      for (const UpdateNode *un = read->updates.head.get(); un; un = un->next.get())
        us.push_back(un);

      for (auto it = us.rbegin(); it != us.rend(); it++) {
        const UpdateNode *un = *it;
        auto ce = dyn_cast<ConstantExpr>(un->index);
        assert(ce && "Not a constant expression");
        uint64_t index = ce->getAPValue().getLimitedValue();
        if (!isa<ConstantExpr>(un->value)) {
          ba.set(index);
        } else {
          ba.unset(index);
          auto arrayValue =
              dyn_cast<ConstantExpr>(un->value);
          assert(arrayValue && "Not a constant expression");
          arrayConstValues[index] = arrayValue;
        }
      }

      std::vector<std::pair<uint64_t, bool>> arrayValues;
      unsigned symByteNum = 0;
      for (unsigned i = 0; i < elementsInArray; i++) {
        uint64_t val = 0;
        bool elementIsConcrete = true;
        for (unsigned j = 0; j < bytesPerElement; j++) {
          if (ba.get((i * bytesPerElement) + j)) {
            elementIsConcrete = false;
            break;
          } else {
            val |= (*(
                         arrayConstValues[(i * bytesPerElement) + j]
                             .get()
                             ->getAPValue()
                             .getRawData())
                    << (j * 8));
          }
        }
        if (elementIsConcrete) {
          arrayValues.emplace_back(val, true);
        } else {
          symByteNum++;
          arrayValues.emplace_back(0, false);
        }
      }

      if (((double)symByteNum / (double)elementsInArray) <=
          ArrayValueSymbRatio) {
        // If the optimization can be applied we apply it
        // Build the dynamic select expression
        ref<Expr> opt =
            buildMixedSelectExpr(read, arrayValues, width, elementsInArray);
        if (opt.get()) {
          cacheReadExprOptimized[const_cast<ReadExpr *>(read)] = opt;
          optimized.insert(std::make_pair(info.first, opt));
        }
      }
    }
    ArrayValueOptReplaceVisitor replacer(optimized, false);
    toReturn = replacer.visit(e);
  }

  return toReturn.get() ? toReturn : notFound;
}

ref<Expr> ExprOptimizer::buildConstantSelectExpr(
    const ref<Expr> &index, std::vector<uint64_t> &arrayValues,
    Expr::Width width, unsigned arraySize) const {
  std::vector<std::pair<uint64_t, uint64_t>> ranges;
  std::vector<uint64_t> values;
  std::set<uint64_t> unique_array_values;
  ExprBuilder *builder = createDefaultExprBuilder();
  Expr::Width valWidth = width;
  ref<Expr> result;

  ref<Expr> actualIndex;
  if (index->getWidth() > Expr::Int32) {
    actualIndex = ExtractExpr::alloc(index, 0, Expr::Int32);
  } else {
    actualIndex = index;
  }
  Expr::Width idxWidth = actualIndex->getWidth();

  // Calculate the repeating values ranges in the constant array
  unsigned curr_idx = 0;
  uint64_t curr_val = arrayValues[0];
  for (unsigned i = 0; i < arraySize; i++) {
    uint64_t temp = arrayValues[i];
    unique_array_values.insert(curr_val);
    if (temp != curr_val) {
      ranges.emplace_back(curr_idx, i);
      values.push_back(curr_val);
      curr_val = temp;
      curr_idx = i;
      if (i == (arraySize - 1)) {
        ranges.emplace_back(curr_idx, i + 1);
        values.push_back(curr_val);
      }
    } else if (i == (arraySize - 1)) {
      ranges.emplace_back(curr_idx, i + 1);
      values.push_back(curr_val);
    }
  }

  if (((double)unique_array_values.size() / (double)(arraySize)) >=
      ArrayValueRatio) {
    return result;
  }

  std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> exprMap;
  for (size_t i = 0; i < ranges.size(); i++) {
    if (exprMap.find(values[i]) != exprMap.end()) {
      exprMap[values[i]].emplace_back(ranges[i].first, ranges[i].second);
    } else {
      if (exprMap.find(values[i]) == exprMap.end()) {
        exprMap.insert(std::make_pair(
            values[i], std::vector<std::pair<uint64_t, uint64_t>>()));
      }
      exprMap.find(values[i])->second.emplace_back(ranges[i].first,
                                                   ranges[i].second);
    }
  }

  int ct = 0;
  // For each range appropriately build the Select expression.
  for (auto range : exprMap) {
    ref<Expr> temp;
    if (ct == 0) {
      temp = builder->Constant(llvm::APInt(valWidth, range.first, false));
    } else {
      if (range.second.size() == 1) {
        if (range.second[0].first == (range.second[0].second - 1)) {
          temp = SelectExpr::create(
              EqExpr::create(actualIndex,
                             builder->Constant(llvm::APInt(
                                 idxWidth, range.second[0].first, false))),
              builder->Constant(llvm::APInt(valWidth, range.first, false)),
              result);

        } else {
          temp = SelectExpr::create(
              AndExpr::create(
                  SgeExpr::create(actualIndex,
                                  builder->Constant(llvm::APInt(
                                      idxWidth, range.second[0].first, false))),
                  SltExpr::create(
                      actualIndex,
                      builder->Constant(llvm::APInt(
                          idxWidth, range.second[0].second, false)))),
              builder->Constant(llvm::APInt(valWidth, range.first, false)),
              result);
        }

      } else {
        ref<Expr> currOr;
        if (range.second[0].first == (range.second[0].second - 1)) {
          currOr = EqExpr::create(actualIndex,
                                  builder->Constant(llvm::APInt(
                                      idxWidth, range.second[0].first, false)));
        } else {
          currOr = AndExpr::create(
              SgeExpr::create(actualIndex,
                              builder->Constant(llvm::APInt(
                                  idxWidth, range.second[0].first, false))),
              SltExpr::create(actualIndex,
                              builder->Constant(llvm::APInt(
                                  idxWidth, range.second[0].second, false))));
        }
        for (size_t i = 1; i < range.second.size(); i++) {
          ref<Expr> tempOr;
          if (range.second[i].first == (range.second[i].second - 1)) {
            tempOr = OrExpr::create(
                EqExpr::create(actualIndex,
                               builder->Constant(llvm::APInt(
                                   idxWidth, range.second[i].first, false))),
                currOr);

          } else {
            tempOr = OrExpr::create(
                AndExpr::create(
                    SgeExpr::create(
                        actualIndex,
                        builder->Constant(llvm::APInt(
                            idxWidth, range.second[i].first, false))),
                    SltExpr::create(
                        actualIndex,
                        builder->Constant(llvm::APInt(
                            idxWidth, range.second[i].second, false)))),
                currOr);
          }
          currOr = tempOr;
        }
        temp = SelectExpr::create(currOr, builder->Constant(llvm::APInt(
                                              valWidth, range.first, false)),
                                  result);
      }
    }
    result = temp;
    ct++;
  }

  delete (builder);

  return result;
}

ref<Expr> ExprOptimizer::buildMixedSelectExpr(
    const ReadExpr *re, std::vector<std::pair<uint64_t, bool>> &arrayValues,
    Expr::Width width, unsigned elementsInArray) const {
  ExprBuilder *builder = createDefaultExprBuilder();
  std::vector<uint64_t> values;
  std::vector<std::pair<uint64_t, uint64_t>> ranges;
  std::vector<uint64_t> holes;
  std::set<uint64_t> unique_array_values;

  unsigned arraySize = elementsInArray;
  unsigned curr_idx = 0;
  uint64_t curr_val = arrayValues[0].first;

  bool emptyRange = true;
  // Calculate Range values
  for (size_t i = 0; i < arrayValues.size(); i++) {
    // If the value is concrete
    if (arrayValues[i].second) {
      // The range contains a concrete value
      emptyRange = false;
      uint64_t temp = arrayValues[i].first;
      unique_array_values.insert(temp);
      if (temp != curr_val) {
        ranges.emplace_back(curr_idx, i);
        values.push_back(curr_val);
        curr_val = temp;
        curr_idx = i;
        if (i == (arraySize - 1)) {
          ranges.emplace_back(curr_idx, curr_idx + 1);
          values.push_back(curr_val);
        }
      } else if (i == (arraySize - 1)) {
        ranges.emplace_back(curr_idx, i + 1);
        values.push_back(curr_val);
      }
    } else {
      holes.push_back(i);
      // If this is not an empty range
      if (!emptyRange) {
        ranges.emplace_back(curr_idx, i);
        values.push_back(curr_val);
      }
      curr_val = arrayValues[i + 1].first;
      curr_idx = i + 1;
      emptyRange = true;
    }
  }

  assert(!unique_array_values.empty() && "No unique values");
  assert(!ranges.empty() && "No ranges");

  ref<Expr> result;
  if (((double)unique_array_values.size() / (double)(arraySize)) <=
      ArrayValueRatio) {
    // The final "else" expression will be the original unoptimized array read
    // expression
    unsigned range_start = 0;
    if (holes.empty()) {
      result = builder->Constant(llvm::APInt(width, values[0], false));
      range_start = 1;
    } else {
      ref<Expr> firstIndex = MulExpr::create(
          ConstantExpr::create(holes[0], re->index->getWidth()),
          ConstantExpr::create(width / 8, re->index->getWidth()));
      result = extendRead(re->updates, firstIndex, width);
      for (size_t i = 1; i < holes.size(); i++) {
        ref<Expr> temp_idx = MulExpr::create(
            ConstantExpr::create(holes[i], re->index->getWidth()),
            ConstantExpr::create(width / 8, re->index->getWidth()));
        ref<Expr> cond = EqExpr::create(re->index, temp_idx);
        ref<Expr> temp = SelectExpr::create(
            cond, extendRead(re->updates, temp_idx, width), result);
        result = temp;
      }
    }

    ref<Expr> new_index = UDivExpr::create(
        re->index, ConstantExpr::create(width / 8, re->index->getWidth()));

    int new_index_width = new_index->getWidth();
    // Iterate through all the ranges
    for (size_t i = range_start; i < ranges.size(); i++) {
      ref<Expr> temp;
      if (ranges[i].second - 1 == ranges[i].first) {
        ref<Expr> cond = EqExpr::create(
            new_index, ConstantExpr::create(ranges[i].first, new_index_width));
        ref<Expr> t = ConstantExpr::create(values[i], width);
        ref<Expr> f = result;
        temp = SelectExpr::create(cond, t, f);
      } else {
        // Create the select constraint
        ref<Expr> cond = AndExpr::create(
            SgeExpr::create(new_index, ConstantExpr::create(ranges[i].first,
                                                            new_index_width)),
            SltExpr::create(new_index, ConstantExpr::create(ranges[i].second,
                                                            new_index_width)));
        ref<Expr> t = ConstantExpr::create(values[i], width);
        ref<Expr> f = result;
        temp = SelectExpr::create(cond, t, f);
      }
      result = temp;
    }
  }

  delete (builder);

  return result;
}
