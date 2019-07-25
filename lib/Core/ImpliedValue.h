//===-- ImpliedValue.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_IMPLIEDVALUE_H
#define KLEE_IMPLIEDVALUE_H

#include "klee/Expr/Expr.h"

#include <vector>

// The idea of implied values is that often we know the result of some
// expression e is a concrete value C. In many cases this directly
// implies that some variable x embedded in e is also a concrete value
// (derived from C). This module is used for finding such variables
// and their computed values.

namespace klee {
  class ConstantExpr;
  class Expr;
  class ReadExpr;
  class Solver;

  typedef std::vector< std::pair<ref<ReadExpr>, 
                                 ref<ConstantExpr> > > ImpliedValueList;
  
  namespace ImpliedValue {        
    void getImpliedValues(ref<Expr> e, ref<ConstantExpr> cvalue, 
                          ImpliedValueList &result);
    void checkForImpliedValues(Solver *S, ref<Expr> e, 
                               ref<ConstantExpr> cvalue);    
  }

}

#endif /* KLEE_IMPLIEDVALUE_H */
