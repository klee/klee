//===-- ArrayExprVisitor.h ------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_ARRAYEXPRVISITOR_H
#define KLEE_ARRAYEXPRVISITOR_H

#include "klee/Expr/ExprBuilder.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Solver/SolverCmdLine.h"

#include <unordered_map>
#include <unordered_set>

namespace klee {

//------------------------------ HELPER FUNCTIONS ---------------------------//
class ArrayExprHelper {
private:
  static bool isReadExprAtOffset(ref<Expr> e, const ReadExpr *base,
                                 ref<Expr> offset);

public:
  static ReadExpr *hasOrderedReads(const ConcatExpr &ce);
};

//--------------------------- INDEX-BASED OPTIMIZATION-----------------------//
class ConstantArrayExprVisitor : public ExprVisitor {
private:
  using bindings_ty = std::map<const Array *, std::vector<ref<Expr>>>;
  bindings_ty &arrays;
  // Avoids adding the same index twice
  std::unordered_set<unsigned> addedIndexes;
  bool incompatible;

protected:
  Action visitConcat(const ConcatExpr &) override;
  Action visitRead(const ReadExpr &) override;

public:
  explicit ConstantArrayExprVisitor(bindings_ty &_arrays)
      : arrays(_arrays), incompatible(false) {}
  inline bool isIncompatible() { return incompatible; }
};

class IndexCompatibilityExprVisitor : public ExprVisitor {
private:
  bool compatible{true};
  bool inner{false};

protected:
  Action visitRead(const ReadExpr &) override;
  Action visitURem(const URemExpr &) override;
  Action visitSRem(const SRemExpr &) override;
  Action visitOr(const OrExpr &) override;

public:
  IndexCompatibilityExprVisitor() = default;

  inline bool isCompatible() { return compatible; }
  inline bool hasInnerReads() { return inner; }
};

class IndexTransformationExprVisitor : public ExprVisitor {
private:
  const Array *array;
  Expr::Width width;
  ref<Expr> mul;

protected:
  Action visitConcat(const ConcatExpr &) override;
  Action visitMul(const MulExpr &) override;

public:
  explicit IndexTransformationExprVisitor(const Array *_array)
      : array(_array), width(Expr::InvalidWidth) {}

  inline Expr::Width getWidth() {
    return width == Expr::InvalidWidth ? Expr::Int8 : width;
  }
  inline ref<Expr> getMul() { return mul; }
};

//------------------------- VALUE-BASED OPTIMIZATION-------------------------//
class ArrayReadExprVisitor : public ExprVisitor {
private:
  std::vector<const ReadExpr *> &reads;
  std::map<const ReadExpr *, std::pair<unsigned, Expr::Width>> &readInfo;
  bool symbolic;
  bool incompatible;

  Action inspectRead(unsigned hash, Expr::Width width, const ReadExpr &);

protected:
  Action visitConcat(const ConcatExpr &) override;
  Action visitRead(const ReadExpr &) override;

public:
  ArrayReadExprVisitor(
      std::vector<const ReadExpr *> &_reads,
      std::map<const ReadExpr *, std::pair<unsigned, Expr::Width>> &_readInfo)
      : ExprVisitor(true), reads(_reads), readInfo(_readInfo), symbolic(false),
        incompatible(false) {}
  inline bool isIncompatible() { return incompatible; }
  inline bool containsSymbolic() { return symbolic; }
};

class ArrayValueOptReplaceVisitor : public ExprVisitor {
private:
  std::unordered_set<unsigned> visited;
  std::map<unsigned, ref<Expr>> optimized;

protected:
  Action visitConcat(const ConcatExpr &) override;
  Action visitRead(const ReadExpr &re) override;

public:
  explicit ArrayValueOptReplaceVisitor(
      std::map<unsigned, ref<Expr>> &_optimized, bool recursive = true)
      : ExprVisitor(recursive), optimized(_optimized) {}
};

class IndexCleanerVisitor : public ExprVisitor {
private:
  bool mul{true};
  ref<Expr> index;

protected:
  Action visitMul(const MulExpr &) override;
  Action visitRead(const ReadExpr &) override;

public:
  IndexCleanerVisitor() : ExprVisitor(true) {}
  inline ref<Expr> getIndex() { return index; }
};
} // namespace klee

#endif /* KLEE_ARRAYEXPRVISITOR_H */
