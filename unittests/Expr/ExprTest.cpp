//===-- ExprTest.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include "gtest/gtest.h"

#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/Expr.h"

using namespace klee;

namespace {

ref<Expr> getConstant(int value, Expr::Width width) {
  int64_t ext = value;
  uint64_t trunc = ext & (((uint64_t) -1LL) >> (64 - width));
  return ConstantExpr::create(trunc, width);
}

TEST(ExprTest, BasicConstruction) {
  EXPECT_EQ(ref<Expr>(ConstantExpr::alloc(0, 32)),
            SubExpr::create(ConstantExpr::alloc(10, 32),
                            ConstantExpr::alloc(10, 32)));
}

TEST(ExprTest, ConcatExtract) {
  ArrayCache ac;
  const Array *array = ac.CreateArray("arr0", 256);
  ref<Expr> read8 = Expr::createTempRead(array, 8);
  const Array *array2 = ac.CreateArray("arr1", 256);
  ref<Expr> read8_2 = Expr::createTempRead(array2, 8);
  ref<Expr> c100 = getConstant(100, 8);

  ref<Expr> concat1 = ConcatExpr::create4(read8, read8, c100, read8_2);
  EXPECT_EQ(2U, concat1->getNumKids());
  EXPECT_EQ(2U, concat1->getKid(1)->getNumKids());
  EXPECT_EQ(2U, concat1->getKid(1)->getKid(1)->getNumKids());

  ref<Expr> extract1 = ExtractExpr::create(concat1, 8, 16);
  EXPECT_EQ(Expr::Concat, extract1->getKind());
  EXPECT_EQ(read8, extract1->getKid(0));
  EXPECT_EQ(c100, extract1->getKid(1));

  ref<Expr> extract2 = ExtractExpr::create(concat1, 6, 26);
  EXPECT_EQ( Expr::Concat, extract2->getKind());
  EXPECT_EQ( read8, extract2->getKid(0));
  EXPECT_EQ( Expr::Concat, extract2->getKid(1)->getKind());
  EXPECT_EQ( read8, extract2->getKid(1)->getKid(0));
  EXPECT_EQ( Expr::Concat, extract2->getKid(1)->getKid(1)->getKind());
  EXPECT_EQ( c100, extract2->getKid(1)->getKid(1)->getKid(0));
  EXPECT_EQ( Expr::Extract, extract2->getKid(1)->getKid(1)->getKid(1)->getKind());
  
  ref<Expr> extract3 = ExtractExpr::create(concat1, 24, 1);
  EXPECT_EQ(Expr::Extract, extract3->getKind());

  ref<Expr> extract4 = ExtractExpr::create(concat1, 27, 2);
  EXPECT_EQ(Expr::Extract, extract4->getKind());
  const ExtractExpr* tmp = cast<ExtractExpr>(extract4);
  EXPECT_EQ(3U, tmp->offset);
  EXPECT_EQ(2U, tmp->getWidth());

  ref<Expr> extract5 = ExtractExpr::create(concat1, 17, 5);
  EXPECT_EQ(Expr::Extract, extract5->getKind());

  ref<Expr> extract6 = ExtractExpr::create(concat1, 3, 26);
  EXPECT_EQ(Expr::Concat, extract6->getKind());
  EXPECT_EQ(Expr::Extract, extract6->getKid(0)->getKind());
  EXPECT_EQ(Expr::Concat, extract6->getKid(1)->getKind());
  EXPECT_EQ(read8, extract6->getKid(1)->getKid(0));
  EXPECT_EQ(Expr::Concat, extract6->getKid(1)->getKid(1)->getKind());
  EXPECT_EQ(c100, extract6->getKid(1)->getKid(1)->getKid(0));
  EXPECT_EQ(Expr::Extract, extract6->getKid(1)->getKid(1)->getKid(1)->getKind());

  ref<Expr> concat10 = ConcatExpr::create4(read8, c100, c100, read8);    
  ref<Expr> extract10 = ExtractExpr::create(concat10, 8, 16);
  EXPECT_EQ(Expr::Constant, extract10->getKind());
}

TEST(ExprTest, ExtractConcat) {
  ArrayCache ac;
  const Array *array = ac.CreateArray("arr2", 256);
  ref<Expr> read64 = Expr::createTempRead(array, 64);

  const Array *array2 = ac.CreateArray("arr3", 256);
  ref<Expr> read8_2 = Expr::createTempRead(array2, 8);
  
  ref<Expr> extract1 = ExtractExpr::create(read64, 36, 4);
  ref<Expr> extract2 = ExtractExpr::create(read64, 32, 4);
  
  ref<Expr> extract3 = ExtractExpr::create(read64, 12, 3);
  ref<Expr> extract4 = ExtractExpr::create(read64, 10, 2);
  ref<Expr> extract5 = ExtractExpr::create(read64, 2, 8);
   
  ref<Expr> kids1[6] = { extract1, extract2,
			 read8_2,
			 extract3, extract4, extract5 };
  ref<Expr> concat1 = ConcatExpr::createN(6, kids1);
  EXPECT_EQ(29U, concat1->getWidth());
  
  ref<Expr> extract6 = ExtractExpr::create(read8_2, 2, 5);
  ref<Expr> extract7 = ExtractExpr::create(read8_2, 1, 1);
  
  ref<Expr> kids2[3] = { extract1, extract6, extract7 };
  ref<Expr> concat2 = ConcatExpr::createN(3, kids2);
  EXPECT_EQ(10U, concat2->getWidth());
  EXPECT_EQ(Expr::Extract, concat2->getKid(0)->getKind());
  EXPECT_EQ(Expr::Extract, concat2->getKid(1)->getKind());
}

TEST(ExprTest, ReadExprFoldingBasic) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Basic constant folding rule
  UpdateList ul(array, 0);
  ref<Expr> read;

  for (unsigned i = 0; i < size; ++i) {
    // Constant index (i)
    read = ReadExpr::create(ul, ConstantExpr::create(i, Expr::Int32));
    EXPECT_EQ(Expr::Constant, read.get()->getKind());
    // Read - should be constant folded to Contents[i]
    // Check that constant folding worked
    ConstantExpr *c = static_cast<ConstantExpr *>(read.get());
    EXPECT_EQ(Contents[i]->getZExtValue(), c->getZExtValue());
  }
}

TEST(ExprTest, ReadExprFoldingIndexOutOfBound) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with index-out-of-bound
  // Constant index (128)
  ref<Expr> index = ConstantExpr::create(128, Expr::Int32);
  UpdateList ul(array, 0);
  ref<Expr> read = ReadExpr::create(ul, index);
  // Read - should not be constant folded
  // Check that constant folding was not applied
  EXPECT_EQ(Expr::Read, read.get()->getKind());
}

TEST(ExprTest, ReadExprFoldingConstantUpdate) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with constant update
  // Constant index (0)
  ref<Expr> index = ConstantExpr::create(0, Expr::Int32);
  UpdateList ul(array, 0);
  ref<Expr> updateValue = ConstantExpr::create(32, Expr::Int8);
  ul.extend(index, updateValue);
  ref<Expr> read = ReadExpr::create(ul, index);
  // Read - should be constant folded to 32
  // Check that constant folding worked
  EXPECT_EQ(Expr::Constant, read.get()->getKind());
  ConstantExpr *c = static_cast<ConstantExpr *>(read.get());
  EXPECT_EQ(UINT64_C(32), c->getZExtValue());
}

TEST(ExprTest, ReadExprFoldingConstantMultipleUpdate) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with constant update
  // Constant index (0)
  ref<Expr> index = ConstantExpr::create(0, Expr::Int32);
  UpdateList ul(array, 0);
  ref<Expr> updateValue = ConstantExpr::create(32, Expr::Int8);
  ref<Expr> updateValue2 = ConstantExpr::create(64, Expr::Int8);
  ul.extend(index, updateValue);
  ul.extend(index, updateValue2);
  ref<Expr> read = ReadExpr::create(ul, index);
  // Read - should be constant folded to 64
  // Check that constant folding worked
  EXPECT_EQ(Expr::Constant, read.get()->getKind());
  ConstantExpr *c = static_cast<ConstantExpr *>(read.get());
  EXPECT_EQ(UINT64_C(64), c->getZExtValue());
}

TEST(ExprTest, ReadExprFoldingSymbolicValueUpdate) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with symbolic update (value)
  // Constant index (0)
  ref<Expr> index = ConstantExpr::create(0, Expr::Int32);
  UpdateList ul(array, 0);
  const Array *array2 = ac.CreateArray("arr2", 256);
  ref<Expr> updateValue = ReadExpr::createTempRead(array2, Expr::Int8);
  ul.extend(index, updateValue);
  ref<Expr> read = ReadExpr::create(ul, index);
  // Read - should be constant folded to the symbolic value
  // Check that constant folding worked
  EXPECT_EQ(Expr::Read, read.get()->getKind());
  EXPECT_EQ(updateValue, read);
}

TEST(ExprTest, ReadExprFoldingSymbolicIndexUpdate) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with symbolic update (index)
  UpdateList ul(array, 0);
  const Array *array2 = ac.CreateArray("arr2", 256);
  ref<Expr> updateIndex = ReadExpr::createTempRead(array2, Expr::Int32);
  ref<Expr> updateValue = ConstantExpr::create(12, Expr::Int8);
  ul.extend(updateIndex, updateValue);
  ref<Expr> read;

  for (unsigned i = 0; i < size; ++i) {
    // Constant index (i)
    read = ReadExpr::create(ul, ConstantExpr::create(i, Expr::Int32));
    // Read - should not be constant folded
    // Check that constant folding was not applied
    EXPECT_EQ(Expr::Read, read.get()->getKind());
  }
}
}
