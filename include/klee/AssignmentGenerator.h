#ifndef KLEE_ASSIGNMENTGENERATOR_H
#define KLEE_ASSIGNMENTGENERATOR_H

#include <cassert>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Expr.h"
#include "Constraints.h"
#include "Internal/Support/ErrorHandling.h"
#include "util/ArrayExprVisitor.h"
#include "util/Assignment.h"
#include "util/Ref.h"

#include "llvm/Support/TimeValue.h"
#include "klee/Internal/System/Time.h"

namespace klee {

class Expr;
template <class T> class ref;

class AssignmentGenerator {
public:
  static bool generatePartialAssignment(const ref<Expr> &e, ref<Expr> &val,
                                        Assignment *&a);

private:
  static bool helperGenerateAssignment(const ref<Expr> &e, ref<Expr> &val,
                                       Assignment *&a, Expr::Width width,
                                       bool sign);

  static bool isReadExprAtOffset(ref<Expr> e, const ReadExpr *base,
                                 ref<Expr> offset);
  static ReadExpr *hasOrderedReads(ref<Expr> e);

  static ref<Expr> createSubExpr(const ref<Expr> &l, ref<Expr> &r);
  static ref<Expr> createAddExpr(const ref<Expr> &l, ref<Expr> &r);
  static ref<Expr> createMulExpr(const ref<Expr> &l, ref<Expr> &r);
  static ref<Expr> createDivExpr(const ref<Expr> &l, ref<Expr> &r, bool sign);
  static ref<Expr> createDivRem(const ref<Expr> &l, ref<Expr> &r, bool sign);
  static ref<Expr> createShlExpr(const ref<Expr> &l, ref<Expr> &r);
  static ref<Expr> createLShrExpr(const ref<Expr> &l, ref<Expr> &r);
  static ref<Expr> createAndExpr(const ref<Expr> &l, ref<Expr> &r);
  static ref<Expr> createExtractExpr(const ref<Expr> &l, ref<Expr> &r);
  static ref<Expr> createExtendExpr(const ref<Expr> &l, ref<Expr> &r);

  static std::vector<unsigned char> getByteValue(ref<Expr> &val);
  static std::vector<unsigned char>
  getIndexedValue(const std::vector<unsigned char> &c_val, ConstantExpr &index,
                  const unsigned int size);
};
}

#endif
