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
int finished_counter = 0;

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
struct SelfRefExpr {
  /// @brief Required by klee::ref-managed objects
  struct klee::ReferenceCounter __refCount;
  ref<SelfRefExpr> next_;

  explicit SelfRefExpr(ref<SelfRefExpr> next) : next_(next) {}
  SelfRefExpr(const SelfRefExpr &) = delete;
  SelfRefExpr &operator=(const SelfRefExpr &) = delete;

  ~SelfRefExpr() { finished_counter++; }
};
TEST(RefTest, SelfRef) {
  struct SelfRefExpr *e_1 = new SelfRefExpr(nullptr);
  ref<SelfRefExpr> r_e_1(e_1);
  EXPECT_EQ(1u, r_e_1->__refCount.refCount);
  ref<SelfRefExpr> r_root = r_e_1;
  EXPECT_EQ(2u, r_e_1->__refCount.refCount);

  {
    ref<SelfRefExpr> r2(new SelfRefExpr(r_e_1));
    EXPECT_EQ(3u, r_e_1->__refCount.refCount);

    r_root = r2;
    EXPECT_EQ(2u, r_e_1->__refCount.refCount);
  }

  r_root = r_root->next_;
  EXPECT_EQ(2u, r_e_1->__refCount.refCount);
}