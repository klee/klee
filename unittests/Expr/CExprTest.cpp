//===--  --------------------------------------------------*- C -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements tests for the C bindings of the klee expression library.
// This is achieved by constructing structurally equivalent expressions with
// both the C and the C++ APIs and comparing the underlying C++ objects.
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"
#include <iostream>

#include "klee-c/expr.h"
#include "klee/Internal/Support/CBindingWrapping.h"

#include "klee/Expr.h"
#include "klee/util/ArrayCache.h"


using namespace klee;

// This needs to be consistent with whatever is in expr.cpp
KLEE_DEFINE_C_WRAPPERS(ref<Expr>, klee_expr_t)
KLEE_DEFINE_C_WRAPPERS(Array, klee_array_t)
KLEE_DEFINE_C_WRAPPERS(UpdateList, klee_update_list_t)

namespace {

class CExprTest : public ::testing::Test {
protected:
  void SetUp() override {
    Builder = klee_expr_builder_create();
  }
  void TearDown() override {
    klee_expr_builder_dispose(Builder);
  }

  klee_expr_builder_t Builder;
};

TEST_F(CExprTest, BasicConstantConstruction) {
  ref<Expr> Expr = ConstantExpr::alloc(0, 32);
  auto CExpr = klee_expr_build_constant(Builder, 0, 32, false);
  auto UnwrappedCExpr = unwrap(CExpr);
  EXPECT_EQ(Expr, *UnwrappedCExpr);
}

TEST_F(CExprTest, BasicComparison) {
  auto Expr1 = klee_expr_build_constant(Builder, 0, 32, false);
  auto Expr2 = klee_expr_build_constant(Builder, 0, 32, false);
  EXPECT_EQ(0, klee_expr_compare(Expr1, Expr2));
}

} // namespace
