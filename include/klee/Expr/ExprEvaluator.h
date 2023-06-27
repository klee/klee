//===-- ExprEvaluator.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPREVALUATOR_H
#define KLEE_EXPREVALUATOR_H

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprVisitor.h"

namespace klee {
class ExprEvaluator : public ExprVisitor {
protected:
  Action evalRead(const UpdateList &ul, unsigned index);
  Action visitRead(const ReadExpr &re) override;
  Action visitSelect(const SelectExpr &se) override;
  Action visitExpr(const Expr &e) override;

  Action protectedDivOperation(const BinaryExpr &e);
  Action visitUDiv(const UDivExpr &e) override;
  Action visitSDiv(const SDivExpr &e) override;
  Action visitURem(const URemExpr &e) override;
  Action visitSRem(const SRemExpr &e) override;
  Action visitExprPost(const Expr &e) override;

public:
  ExprEvaluator() {}

  /// getInitialValue - Return the initial value for a symbolic byte.
  ///
  /// This will only be called for constant arrays if the index is
  /// out-of-bounds. If the value is unknown then the user should return a
  /// ReadExpr at the initial version of this array.
  virtual ref<Expr> getInitialValue(const Array &os, unsigned index) = 0;
};
} // namespace klee

#endif /* KLEE_EXPREVALUATOR_H */
