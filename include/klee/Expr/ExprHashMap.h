//===-- ExprHashMap.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPRHASHMAP_H
#define KLEE_EXPRHASHMAP_H

#include "klee/Expr/Expr.h"

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

  namespace util {
    struct ExprHash  {
      unsigned operator()(const ref<Expr> e) const {
        return e->hash();
      }
    };
    
    struct ExprCmp {
      bool operator()(const ref<Expr> &a, const ref<Expr> &b) const {
        return a==b;
      }
    };
  }
  
  template<class T> 
  class ExprHashMap : 

    public unordered_map<ref<Expr>,
                         T,
                         klee::util::ExprHash,
                         klee::util::ExprCmp> {
  };
  
  typedef unordered_set<ref<Expr>,
                        klee::util::ExprHash,
                        klee::util::ExprCmp> ExprHashSet;

}

#undef unordered_map
#undef unordered_set

#endif /* KLEE_EXPRHASHMAP_H */
