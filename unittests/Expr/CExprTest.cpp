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
#include <array>
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
  void SetUp() override { Builder = klee_expr_builder_create(); }
  void TearDown() override { klee_expr_builder_dispose(Builder); }

  klee_expr_builder_t Builder;
};

TEST_F(CExprTest, BasicConstantConstruction) {
  ref<Expr> Expr = ConstantExpr::alloc(0, 32);
  auto CExpr = klee_build_constant_expr(Builder, 0, 32, false);
  auto UnwrappedCExpr = unwrap(CExpr);
  EXPECT_EQ(Expr, *UnwrappedCExpr);

  klee_expr_dispose(CExpr);
}

TEST_F(CExprTest, BasicComparison) {
  auto Expr1 = klee_build_constant_expr(Builder, 0, 32, false);
  auto Expr2 = klee_build_constant_expr(Builder, 0, 32, false);
  EXPECT_EQ(0, klee_expr_compare(Expr1, Expr2));

  klee_expr_dispose(Expr1);
  klee_expr_dispose(Expr2);
}

TEST_F(CExprTest, BasicRead) {
  auto CArray = klee_array_create(Builder, "arr0", 4, NULL, false, 32, 8);
  auto *Array = unwrap(CArray);

  auto ReadExpr = Expr::createTempRead(Array, Expr::Int8);

  auto CUL = klee_update_list_create(CArray);
  auto CIndexExpr = klee_build_constant_expr(Builder, 0, 32, false);
  auto CReadExpr = klee_build_read_expr(Builder, CUL, CIndexExpr);

  auto *UnwrappedReadExpr = unwrap(CReadExpr);
  EXPECT_EQ(ReadExpr, *UnwrappedReadExpr);

  klee_expr_dispose(CIndexExpr);
  klee_expr_dispose(CReadExpr);
}

TEST_F(CExprTest, Read) {
  auto CArray = klee_array_create(Builder, "arr0", 256, NULL, false, 32, 8);
  auto *Array = unwrap(CArray);

  auto ReadExpr = Expr::createTempRead(Array, Expr::Int32);

  auto CUL = klee_update_list_create(CArray);
  std::array<klee_expr_t, 4> CReads;
  // Need to loop in reverse to preserve read order in UL
  for (int i = 3; i >= 0; i--) {
    auto CConstantExpr = klee_build_constant_expr(Builder, i, 32, false);
    CReads[i] = klee_build_read_expr(Builder, CUL, CConstantExpr);
    klee_expr_dispose(CConstantExpr);
  }

  // Build a chain of concats unbalanced to the right to match createTempRead
  auto CInnerConcat = klee_build_concat_expr(Builder, CReads[1], CReads[0]);
  auto CMiddleConcat = klee_build_concat_expr(Builder, CReads[2], CInnerConcat);
  auto COuterConcat = klee_build_concat_expr(Builder, CReads[3], CMiddleConcat);

  auto *UnwrappedReadExpr = unwrap(COuterConcat);
  EXPECT_EQ(ReadExpr, *UnwrappedReadExpr);

  klee_expr_dispose(CInnerConcat);
  klee_expr_dispose(CMiddleConcat);
  klee_expr_dispose(COuterConcat);

  for (auto CRead : CReads)
    klee_expr_dispose(CRead);
}

} // namespace
