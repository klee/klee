//===-- ExprUtil.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPRUTIL_H
#define KLEE_EXPRUTIL_H

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprVisitor.h"

#include <set>
#include <unordered_set>
#include <vector>

namespace klee {
class Array;
class Expr;
class ReadExpr;
template <typename T> class ref;

/// Find all ReadExprs used in the expression DAG. If visitUpdates
/// is true then this will including those reachable by traversing
/// update lists. Note that this may be slow and return a large
/// number of results.
void findReads(ref<Expr> e, bool visitUpdates,
               std::vector<ref<ReadExpr>> &result);

/// Return a list of all unique symbolic objects referenced by the given
/// expression.
void findSymbolicObjects(ref<Expr> e, std::vector<const Array *> &results);

/// Return a list of all unique symbolic objects referenced by the
/// given expression range.
template <typename InputIterator>
void findSymbolicObjects(InputIterator begin, InputIterator end,
                         std::vector<const Array *> &results);

template <typename InputIterator>
void findObjects(InputIterator begin, InputIterator end,
                 std::vector<const Array *> &results);

void findObjects(ref<Expr> e, std::vector<const Array *> &results);

bool isReadFromSymbolicArray(ref<Expr> e);

ref<Expr> createNonOverflowingSumExpr(const std::vector<ref<Expr>> &terms);

class ConstantArrayFinder : public ExprVisitor {
protected:
  ExprVisitor::Action visitRead(const ReadExpr &re);

public:
  std::set<const Array *> results;
};
} // namespace klee

#endif /* KLEE_EXPRUTIL_H */
