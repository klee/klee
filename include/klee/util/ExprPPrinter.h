//===-- ExprPPrinter.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPRPPRINTER_H
#define KLEE_EXPRPPRINTER_H

#include "klee/Expr.h"

namespace klee {
  class ConstraintManager;

  class ExprPPrinter {
  protected:
    ExprPPrinter() {}
    
  public:
    static ExprPPrinter *create(std::ostream &os);

    virtual ~ExprPPrinter() {}

    virtual void setNewline(const std::string &newline) = 0;
    virtual void reset() = 0;
    virtual void scan(const ref<Expr> &e) = 0;
    virtual void print(const ref<Expr> &e, unsigned indent=0) = 0;

    // utility methods

    template<class Container>
    void scan(Container c) {
      scan(c.begin(), c.end());
    }

    template<class InputIterator>
    void scan(InputIterator it, InputIterator end) {
      for (; it!=end; ++it)
        scan(*it);
    }

    static void printOne(std::ostream &os, const char *message, 
                         const ref<Expr> &e);

    static void printConstraints(std::ostream &os,
                                 const ConstraintManager &constraints);

    static void printQuery(std::ostream &os,
                           const ConstraintManager &constraints,
                           const ref<Expr> &q);
  };

}

#endif
