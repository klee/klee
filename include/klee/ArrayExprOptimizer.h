#ifndef KLEE_EXPROPTIMIZER_H
#define KLEE_EXPROPTIMIZER_H

#include <cassert>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Expr.h"
#include "ExprBuilder.h"
#include "Constraints.h"
#include "Internal/Support/ErrorHandling.h"
#include "util/ArrayExprVisitor.h"
#include "util/Assignment.h"
#include "util/Ref.h"
#include "klee/AssignmentGenerator.h"
#include "klee/ExecutionState.h"

#include "llvm/Support/TimeValue.h"
#include "klee/Internal/System/Time.h"

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

enum ArrayOptimizationType {
  NONE,
  ALL,
  INDEX,
  VALUE
};

extern llvm::cl::opt<ArrayOptimizationType> OptimizeArray;

extern llvm::cl::opt<double> ArrayValueRatio;

extern llvm::cl::opt<double> ArrayValueSymbRatio;

class Expr;
template <class T> class ref;
typedef std::map<const Array *, std::vector<ref<Expr> > > array2idx_ty;
typedef std::vector<const Array *> v_arr_ty;
typedef std::map<ref<Expr>, std::vector<ref<Expr> > > mapIndexOptimizedExpr_ty;

class ExprOptimizer {
private:
  unordered_map<unsigned, ref<Expr> > cacheExprOptimized;
  unordered_set<unsigned> cacheExprUnapplicable;
  unordered_map<unsigned, ref<Expr> > cacheReadExprOptimized;

public:
  void optimizeExpr(const ref<Expr> &e, ref<Expr> &result,
                    bool valueOnly = false);

private:
  bool computeIndexes(array2idx_ty &arrays, const ref<Expr> &e,
                      std::map<ref<Expr>, std::vector<ref<Expr> > > &idx_valIdx)
      const;

  ref<Expr> getSelectOptExpr(
      const ref<Expr> &e, std::vector<const ReadExpr *> &reads,
      std::map<const ReadExpr *, std::pair<unsigned, Expr::Width> > &readInfo,
      bool isSymbolic);

  ref<Expr> buildConstantSelectExpr(const ref<Expr> &index,
                                   std::vector<uint64_t> &arrayValues,
                                   Expr::Width width, unsigned elementsInArray) const;

  ref<Expr>
  buildMixedSelectExpr(const ReadExpr *re,
                     std::vector<std::pair<uint64_t, bool> > &arrayValues,
                     Expr::Width width, unsigned elementsInArray) const;

};
}

#endif
