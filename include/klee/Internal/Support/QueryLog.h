//===-- QueryLog.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_OPT_LOGGINGSOLVER_H
#define KLEE_OPT_LOGGINGSOLVER_H

#include "klee/Expr.h"

#include <vector>

namespace klee {
  struct Query;

  class QueryLogEntry {
  public:
    enum Type {
      Validity,
      Truth,
      Value,
      Cex
    };
    
    typedef std::vector< ref<Expr> > exprs_ty;
    exprs_ty exprs;

    Type type;
    ref<Expr> query;
    unsigned instruction;
    std::vector<const Array*> objects;
    
  public:
    QueryLogEntry() : query(ConstantExpr::alloc(0,Expr::Bool)) {}
    QueryLogEntry(const QueryLogEntry &b);
    QueryLogEntry(const Query &_query, 
                  Type _type,
                  const std::vector<const Array*> *objects = 0);
  };

  class QueryLogResult {
  public:
    uint64_t result;
    double time;

  public:
    QueryLogResult() {}
    QueryLogResult(bool _success, uint64_t _result, double _time) 
      : result(_result), time(_time) {
      if (!_success) { // la la la
        result = 0;
        time = -1;
      }
    }
  };
  
}

#endif
