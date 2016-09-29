//===-- RefTest.cpp ---------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Regression test for a bug caused by assigning a ref to itself.
   More details at http://keeda.stanford.edu/pipermail/klee-commits/2012-February/000904.html */

#include "gtest/gtest.h"
#include <iostream>
#include "klee/util/Ref.h"
using klee::ref;

int finished = 0;

struct Expr
{
  int refCount;
  Expr() : refCount(0) { 
    //std::cout << "Expr(" << this << ") created\n"; 
  }
  ~Expr() { 
    //std::cout << "Expr(" << this << ") destroyed\n"; 
    EXPECT_EQ(finished, 1);
  }
};

TEST(RefTest, SelfAssign) 
{
  struct Expr *r_e = new Expr();
  ref<Expr> r(r_e);
  EXPECT_EQ(r_e->refCount, 1);
  r = r;
  EXPECT_EQ(r_e->refCount, 1);
  finished = 1;
}
