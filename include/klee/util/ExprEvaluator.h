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

    // override to implement evaluation, this function is called to
    // get the initial value for a symbolic byte. if the value is
    // unknown then the user can simply return a ReadExpr at version 0
    // of this MemoryObject.
    virtual ref<Expr> getInitialValue(const Array& os, unsigned index) = 0;
  };
}

#endif
