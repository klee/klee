#include "klee/Internal/ADT/DiscretePDF.h"
#include "gtest/gtest.h"
#include <iostream>
#include <vector>

int finished = 0;

using namespace klee;

TEST(DiscretePDFTest, Rotatation) {
  DiscretePDF<int> testTree;

  ASSERT_TRUE(testTree.empty());

  for (auto i = 0; i < 9; ++i)
    testTree.insert(i + 10, 0.1 * i);

  for (auto i = 9; i > 0; --i)
    testTree.insert(i + 100, 0.01 * i);

  ASSERT_FALSE(testTree.empty());

  ASSERT_TRUE(testTree.inTree(101));
  testTree.remove(101);
  ASSERT_FALSE(testTree.inTree(101));
  testTree.insert(101, 0.01);
  ASSERT_EQ(0.01, testTree.getWeight(101));

  ASSERT_EQ(11, testTree.choose(0.01));
  testTree.choose(0);
  testTree.choose(0.9999999);

  testTree.update(101, 0.9);

  for (auto i = 0; i < 9; ++i)
    testTree.remove(i + 10);
  for (auto i = 0; i < 50; ++i) {
    testTree.insert(50 + i, 2 * i);
  }

  for (auto i = 9; i > 0; --i)
    testTree.remove(i + 100);

  testTree.insert(1, 0);
#ifndef NDEBUG
  ASSERT_DEATH({ testTree.insert(1, 0); }, "already in tree");
#endif

  while (!testTree.empty()) {
    testTree.remove(testTree.choose(0));
  }

  testTree.insert(1, 1);
  testTree.insert(2, 2);

  ASSERT_EQ(1, testTree.getWeight(1));
  ASSERT_EQ(2, testTree.getWeight(2));
}
