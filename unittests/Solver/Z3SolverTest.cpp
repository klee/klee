//===-- Z3SolverTest.cpp
//----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <algorithm>
#include <iterator>
#include <vector>

#include "gtest/gtest.h"

#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Solver/Solver.h"

using namespace klee;

namespace {
ArrayCache AC;
}
class Z3SolverTest : public ::testing::Test {
protected:
  Z3SolverTest() : Z3Solver_(createCoreSolver(CoreSolverType::Z3_SOLVER)) {
    Z3Solver_->setCoreSolverTimeout(time::Span("10s"));
  }

  virtual ~Z3SolverTest() { delete Z3Solver_; }

  Solver *Z3Solver_;
};

TEST_F(Z3SolverTest, GetConstraintLog) {
  ConstraintSet Constraints;
  ConstraintManager cm(Constraints);

  const std::vector<uint64_t> ConstantValues{1, 2, 3, 4};
  std::vector<ref<ConstantExpr>> ConstantExpressions;

  std::transform(
      ConstantValues.begin(), ConstantValues.end(),
      std::back_inserter(ConstantExpressions), [](const uint64_t Value) {
        ref<ConstantExpr> ConstantExpr(ConstantExpr::alloc(Value, Expr::Int8));
        return ConstantExpr;
      });

  const Array *ConstantArray =
      AC.CreateArray("const_array", 4, ConstantExpressions.data(),
                     ConstantExpressions.data() + ConstantExpressions.size());

  const UpdateList ConstantArrayUL(ConstantArray, nullptr);
  const ref<Expr> Index = ConstantExpr::alloc(1, Expr::Int32);
  const ref<Expr> Lhs = ReadExpr::alloc(ConstantArrayUL, Index);
  const ref<Expr> Rhs = ConstantExpr::alloc(2, Expr::Int8);
  const ref<Expr> Eqn = EqExpr::alloc(Lhs, Rhs);

  Query TheQuery(Constraints, Eqn);

  // Ensure this is not buggy as fixed in https://github.com/klee/klee/pull/1235
  // If the bug is still present this fail due to an internal assertion
  char *ConstraintsString = Z3Solver_->getConstraintLog(TheQuery);
  const char *ExpectedArraySelection = "(= (select const_array0";
  const char *Occurence = std::strstr(ConstraintsString, ExpectedArraySelection);
  ASSERT_STRNE(Occurence, nullptr);
  free(ConstraintsString);
}
