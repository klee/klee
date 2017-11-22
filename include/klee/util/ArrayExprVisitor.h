#ifndef KLEE_ARRAYEXPRVISITOR_H_
#define KLEE_ARRAYEXPRVISITOR_H_

#include "klee/util/ExprVisitor.h"
#include "klee/ExprBuilder.h"
#include "klee/CommandLine.h"

#include <ciso646>
#ifdef _LIBCPP_VERSION
#include <unordered_map>
#include <unordered_set>
#define unordered_map std::unordered_map
#define unordered_set std::unordered_set
#else
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#define unordered_map std::tr1::unordered_map
#define unordered_set std::tr1::unordered_set
#endif

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
  typedef std::map<const Array *, std::vector<ref<Expr> > > bindings_ty;
  bindings_ty &arrays;
  // Avoids adding the same index twice
  unordered_set<unsigned> addedIndexes;
  bool incompatible;

protected:
  Action visitConcat(const ConcatExpr &);
  Action visitRead(const ReadExpr &);

public:
  ConstantArrayExprVisitor(bindings_ty &_arrays)
      : arrays(_arrays), incompatible(false) {}
  inline bool isIncompatible() { return incompatible; }
};

class IndexCompatibilityExprVisitor : public ExprVisitor {
private:
  bool compatible;
  bool inner;

protected:
  Action visitRead(const ReadExpr &);
  Action visitURem(const URemExpr &);
  Action visitSRem(const SRemExpr &);
  Action visitOr(const OrExpr &);

public:
  IndexCompatibilityExprVisitor() : compatible(true), inner(false) {}

  inline bool isCompatible() { return compatible; }
  inline bool hasInnerReads() { return inner; }
};

class IndexTransformationExprVisitor : public ExprVisitor {
private:
  const Array *array;
  Expr::Width width;
  ref<Expr> mul;

protected:
  Action visitConcat(const ConcatExpr &);
  Action visitMul(const MulExpr &);

public:
  IndexTransformationExprVisitor(const Array *_array)
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
  std::map<const ReadExpr *, std::pair<unsigned, Expr::Width> > &readInfo;
  bool symbolic;
  bool incompatible;

  Action inspectRead(unsigned hash, Expr::Width width, const ReadExpr &);

protected:
  Action visitConcat(const ConcatExpr &);
  Action visitRead(const ReadExpr &);

public:
  ArrayReadExprVisitor(
      std::vector<const ReadExpr *> &_reads,
      std::map<const ReadExpr *, std::pair<unsigned, Expr::Width> > &_readInfo)
      : ExprVisitor(true), reads(_reads), readInfo(_readInfo), symbolic(false),
        incompatible(false) {}
  inline bool isIncompatible() { return incompatible; }
  inline bool containsSymbolic() { return symbolic; }
};

class ArrayValueOptReplaceVisitor : public ExprVisitor {
private:
  unordered_set<unsigned> visited;
  std::map<unsigned, ref<Expr> > optimized;

protected:
  Action visitConcat(const ConcatExpr &);
  Action visitRead(const ReadExpr &re);

public:
  ArrayValueOptReplaceVisitor(std::map<unsigned, ref<Expr> > &_optimized,
                              bool recursive = true)
      : ExprVisitor(recursive), optimized(_optimized) {}
};

class IndexCleanerVisitor : public ExprVisitor {
private:
  bool mul;
  ref<Expr> index;

protected:
  Action visitMul(const MulExpr &);
  Action visitRead(const ReadExpr &);

public:
  IndexCleanerVisitor() : ExprVisitor(true), mul(true) {}
  inline ref<Expr> getIndex() { return index; }
};
}

#endif
