//===-- RefTest.cpp ---------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Regression test for a bug caused by assigning a ref to itself.
   More details at
   http://keeda.stanford.edu/pipermail/klee-commits/2012-February/000904.html */

#include "klee/ADT/Ref.h"
#include "gtest/gtest.h"
#include <iostream>

using klee::ref;

int finished = 0;
int finished_counter = 0;

struct Expr {
  /// @brief Required by klee::ref-managed objects
  class klee::ReferenceCounter _refCount;
  Expr() = default;

  Expr(const Expr &) = delete;
  Expr &operator=(const Expr &) = delete;

  ~Expr() {
    // std::cout << "Expr(" << this << ") destroyed\n";
    EXPECT_EQ(finished, 1);
    finished_counter++;
  }
};

struct SelfRefExpr {
  /// @brief Required by klee::ref-managed objects
  class klee::ReferenceCounter _refCount;
  ref<SelfRefExpr> next_;

  explicit SelfRefExpr(ref<SelfRefExpr> next) : next_(next) {}
  SelfRefExpr(const SelfRefExpr &) = delete;
  SelfRefExpr &operator=(const SelfRefExpr &) = delete;

  ~SelfRefExpr() { finished_counter++; }
};

struct ParentExpr {
  /// @brief Required by klee::ref-managed objects
  class klee::ReferenceCounter _refCount;

  enum ExprKind {
    EK_Parent,
    EK_Child,
  };

  const ExprKind Kind;
  ExprKind getKind() const { return Kind; }

  ParentExpr(ExprKind K) : Kind(K) {}

  virtual ~ParentExpr() {
    EXPECT_EQ(finished, 1);
    finished_counter++;
  }
};

struct ChildExpr : public ParentExpr {
  ChildExpr() : ParentExpr(EK_Child) {}

  static bool classof(const ParentExpr *S) { return S->getKind() == EK_Child; }
};

TEST(RefTest, SelfAssign) {
  finished = 0;
  finished_counter = 0;
  {

    struct Expr *r_e = new Expr();
    ref<Expr> r(r_e);
    EXPECT_EQ(r_e->_refCount.getCount(), 1u);
    r = r;
    EXPECT_EQ(r_e->_refCount.getCount(), 1u);
    finished = 1;
  }
  EXPECT_EQ(1, finished_counter);
}

TEST(RefTest, SelfMove) {
  finished = 0;
  finished_counter = 0;
  {
    struct Expr *r_e = new Expr();
    ref<Expr> r(r_e);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
    // Check self move
    r = std::move(r);
#pragma GCC diagnostic pop
    finished = 1;
  }
  EXPECT_EQ(1, finished_counter);
}

TEST(RefTest, MoveAssignment) {
  finished = 0;
  finished_counter = 0;
  {
    struct Expr *r_e = new Expr();
    ref<Expr> r(r_e);

    struct Expr *q_e = new Expr();
    ref<Expr> q(q_e);

    finished = 1;

    // Move the object
    q = std::move(r);

    // Re-assign new object
    r = new Expr();
  }
  EXPECT_EQ(3, finished_counter);
}

TEST(RefTest, MoveCastingAssignment) {
  finished = 0;
  finished_counter = 0;
  {
    ref<ParentExpr> r(new ChildExpr());

    struct ChildExpr *child = new ChildExpr();
    ref<ChildExpr> ce(child);

    finished = 1;

    // Move the object
    r = std::move(child);
  }
  EXPECT_EQ(2, finished_counter);
}

TEST(RefTest, MoveConstructor) {
  finished = 0;
  finished_counter = 0;
  {
    struct Expr *r_e = new Expr();
    ref<Expr> r(r_e);

    ref<Expr> q(std::move(r_e));

    finished = 1;
  }
  EXPECT_EQ(1, finished_counter);
}

TEST(RefTest, SelfRef) {
  struct SelfRefExpr *e_1 = new SelfRefExpr(nullptr);
  ref<SelfRefExpr> r_e_1(e_1);
  EXPECT_EQ(1u, r_e_1->_refCount.getCount());
  ref<SelfRefExpr> r_root = r_e_1;
  EXPECT_EQ(2u, r_e_1->_refCount.getCount());

  {
    ref<SelfRefExpr> r2(new SelfRefExpr(r_e_1));
    EXPECT_EQ(3u, r_e_1->_refCount.getCount());

    r_root = r2;
    EXPECT_EQ(2u, r_e_1->_refCount.getCount());
  }

  r_root = r_root->next_;
  EXPECT_EQ(2u, r_e_1->_refCount.getCount());
}
