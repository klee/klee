//===-- StringModel.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_STRING_MODEL_H
#define KLEE_STRING_MODEL_H

#include "klee/Expr.h"
#include "Memory.h"

#include <vector>
#include <string>
#include <map>
#include <set>

namespace klee {  
typedef std::pair<ref<Expr>, ref<Expr>> StrModel;
 
 class StringModel {
     ref<Expr> one;
     ref<Expr> zero;
     ref<Expr> minusOne;
     ref<Expr> x00;
    
public:
    StringModel();
    /*
    Takes two string  moP and moQ and produces string constraints that model them
    Returns a pair of expression. First is the return value of strcmp.
    Second is the constrain that must hold in the state
    */
    StrModel modelStrcmp(const ObjectState* moP, ref<Expr> p,
                         const ObjectState* moQ, ref<Expr> q);
    StrModel modelStrncmp(const ObjectState* moP, ref<Expr> p,
                         const ObjectState* moQ, ref<Expr> q, ref<Expr> n);

    StrModel modelStrchr(const ObjectState* moS, ref<Expr> s, ref<Expr> c);
    StrModel modelStrlen(const ObjectState* moS, ref<Expr>	s);
    StrModel modelStrcpy(const ObjectState* moDst, ref<Expr> dst,
                         const ObjectState* moSrc, ref<Expr> src);
    StrModel modelStrncpy(const ObjectState* moDst, ref<Expr> dst,
                         const ObjectState* moSrc, ref<Expr> src, ref<Expr> n);

 };
} // End klee namespace

#endif
