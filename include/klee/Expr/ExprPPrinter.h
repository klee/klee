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

#include "klee/Expr/Expr.h"

namespace llvm {
  class raw_ostream;
}
namespace klee {
  class ConstraintSet;

  class ExprPPrinter {
  protected:
    ExprPPrinter() {}
    
  public:
    static ExprPPrinter *create(llvm::raw_ostream &os);

    virtual ~ExprPPrinter() {}

    virtual void setNewline(const std::string &newline) = 0;
    virtual void setForceNoLineBreaks(bool forceNoLineBreaks) = 0;
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

    /// printOne - Pretty print a single expression prefixed by a
    /// message and followed by a line break.
    static void printOne(llvm::raw_ostream &os, const char *message,
                         const ref<Expr> &e);

    /// printSingleExpr - Pretty print a single expression.
    ///
    /// The expression will not be followed by a line break.
    ///
    /// Note that if the output stream is not positioned at the
    /// beginning of a line then printing will not resume at the
    /// correct position following any output line breaks.
    static void printSingleExpr(llvm::raw_ostream &os, const ref<Expr> &e);

    static void printConstraints(llvm::raw_ostream &os,
                                 const ConstraintSet &constraints);

    static void printQuery(llvm::raw_ostream &os,
                           const ConstraintSet &constraints,
                           const ref<Expr> &q,
                           const ref<Expr> *evalExprsBegin = 0,
                           const ref<Expr> *evalExprsEnd = 0,
                           const Array * const* evalArraysBegin = 0,
                           const Array * const* evalArraysEnd = 0,
                           bool printArrayDecls = true);
  };

}

#endif /* KLEE_EXPRPPRINTER_H */
