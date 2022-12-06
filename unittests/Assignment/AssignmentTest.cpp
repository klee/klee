#include "gtest/gtest.h"

#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/SourceBuilder.h"

#include <iostream>
#include <vector>

int finished = 0;

using namespace klee;

TEST(AssignmentTest, FoldNotOptimized) {
  ArrayCache ac;
  const Array *array = ac.CreateArray(
      "simple_array",
      /*size=*/ConstantExpr::create(1, sizeof(uint64_t) * CHAR_BIT),
      SourceBuilder::makeSymbolic());
  // Create a simple assignment
  std::vector<const Array *> objects;
  SparseStorage<unsigned char> value(1);
  std::vector<SparseStorage<unsigned char>> values;

  objects.push_back(array);
  value.store(0, 128);
  values.push_back(value);
  // We want to simplify to a constant so allow free values so
  // if the assignment is incomplete we don't get back a constant.
  Assignment assignment(objects, values, /*_allowFreeValues=*/true);

  // Now make an expression that reads from the array at position
  // zero.
  ref<Expr> read =
      NotOptimizedExpr::alloc(Expr::createTempRead(array, Expr::Int8));

  // Now evaluate. The OptimizedExpr should be folded
  ref<Expr> evaluated = assignment.evaluate(read);
  const ConstantExpr *asConstant = dyn_cast<ConstantExpr>(evaluated);
  ASSERT_TRUE(asConstant != NULL);
  ASSERT_EQ(asConstant->getZExtValue(), (unsigned)128);
}
