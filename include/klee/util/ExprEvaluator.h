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

#include "klee/Expr.h"
#include "klee/util/ExprVisitor.h"

namespace klee {
  class ExprEvaluator : public ExprVisitor {
  protected:
    Action evalRead(const UpdateList &ul, unsigned index);
    Action visitRead(const ReadExpr &re);
    Action visitExpr(const Expr &e);
      
    Action protectedDivOperation(const BinaryExpr &e);
    Action visitUDiv(const UDivExpr &e);
    Action visitSDiv(const SDivExpr &e);
    Action visitURem(const URemExpr &e);
    Action visitSRem(const SRemExpr &e);
      
  public:
    ExprEvaluator() {}

    /// getInitialValue - Return the initial value for a symbolic byte.
    ///
    /// This will only be called for constant arrays if the index is
    /// out-of-bounds. If the value is unknown then the user should return a
    /// ReadExpr at the initial version of this array.
    virtual ref<Expr> getInitialValue(const Array& os, unsigned index) = 0;
  };
}

#endif
