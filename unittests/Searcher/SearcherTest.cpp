//===-- SearcherTest.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Core/BranchTypes.h"
#define KLEE_UNITTEST

#include "gtest/gtest.h"

#include "Core/ExecutionState.h"
#include "Core/PForest.h"
#include "Core/PTree.h"
#include "Core/Searcher.h"
#include "klee/ADT/RNG.h"

#include "llvm/Support/raw_ostream.h"

using namespace klee;

namespace {

TEST(SearcherTest, RandomPath) {
  // First state
  ExecutionState es;
  PForest processForest = PForest();
  processForest.addRoot(&es);
  es.ptreeNode = processForest.getPTrees()
                     .at(es.ptreeNode->getTreeID())
                     ->root.getPointer();

  RNG rng;
  RandomPathSearcher rp(processForest, rng);
  EXPECT_TRUE(rp.empty());

  rp.update(nullptr, {&es}, {});
  EXPECT_FALSE(rp.empty());
  EXPECT_EQ(&rp.selectState(), &es);

  // Two states
  ExecutionState es1(es);
  processForest.attach(es.ptreeNode, &es1, &es, BranchType::NONE);
  rp.update(&es, {&es1}, {});

  // Random path seed dependant
  EXPECT_EQ(&rp.selectState(), &es1);
  EXPECT_EQ(&rp.selectState(), &es);
  EXPECT_EQ(&rp.selectState(), &es1);

  rp.update(&es, {}, {&es1});
  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(&rp.selectState(), &es);
  }

  rp.update(&es, {&es1}, {&es});
  processForest.remove(es.ptreeNode);
  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(&rp.selectState(), &es1);
  }

  rp.update(&es1, {}, {&es1});
  processForest.remove(es1.ptreeNode);
  EXPECT_TRUE(rp.empty());
}

TEST(SearcherTest, TwoRandomPath) {
  // Root state
  ExecutionState root;
  PForest processForest = PForest();
  processForest.addRoot(&root);
  root.ptreeNode = processForest.getPTrees()
                       .at(root.ptreeNode->getTreeID())
                       ->root.getPointer();

  ExecutionState es(root);
  processForest.attach(root.ptreeNode, &es, &root, BranchType::NONE);

  RNG rng, rng1;
  RandomPathSearcher rp(processForest, rng);
  RandomPathSearcher rp1(processForest, rng1);
  EXPECT_TRUE(rp.empty());
  EXPECT_TRUE(rp1.empty());

  rp.update(nullptr, {&es}, {});
  EXPECT_FALSE(rp.empty());
  EXPECT_TRUE(rp1.empty());
  EXPECT_EQ(&rp.selectState(), &es);

  // Two states
  ExecutionState es1(es);
  processForest.attach(es.ptreeNode, &es1, &es, BranchType::NONE);

  rp.update(&es, {}, {});
  rp1.update(nullptr, {&es1}, {});
  EXPECT_FALSE(rp1.empty());

  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(&rp.selectState(), &es);
    EXPECT_EQ(&rp1.selectState(), &es1);
  }

  rp.update(&es, {&es1}, {&es});
  rp1.update(nullptr, {&es}, {&es1});

  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(&rp1.selectState(), &es);
    EXPECT_EQ(&rp.selectState(), &es1);
  }

  rp1.update(&es, {}, {&es});
  processForest.remove(es.ptreeNode);
  EXPECT_TRUE(rp1.empty());
  EXPECT_EQ(&rp.selectState(), &es1);

  rp.update(&es1, {}, {&es1});
  processForest.remove(es1.ptreeNode);
  EXPECT_TRUE(rp.empty());
  EXPECT_TRUE(rp1.empty());

  processForest.remove(root.ptreeNode);
}

TEST(SearcherTest, TwoRandomPathDot) {
  std::stringstream modelPTreeDot;
  PTreeNode *rootPNode, *rightLeafPNode, *esParentPNode, *es1LeafPNode,
      *esLeafPNode;

  // Root state
  ExecutionState root;
  PForest processForest = PForest();
  processForest.addRoot(&root);
  root.ptreeNode = processForest.getPTrees()
                       .at(root.ptreeNode->getTreeID())
                       ->root.getPointer();
  rootPNode = root.ptreeNode;

  ExecutionState es(root);
  processForest.attach(root.ptreeNode, &es, &root, BranchType::NONE);
  rightLeafPNode = root.ptreeNode;
  esParentPNode = es.ptreeNode;

  RNG rng;
  RandomPathSearcher rp(processForest, rng);
  RandomPathSearcher rp1(processForest, rng);

  rp.update(nullptr, {&es}, {});

  ExecutionState es1(es);
  processForest.attach(es.ptreeNode, &es1, &es, BranchType::NONE);
  esLeafPNode = es.ptreeNode;
  es1LeafPNode = es1.ptreeNode;

  rp.update(&es, {}, {});
  rp1.update(nullptr, {&es1}, {});

  // Compare PTree to model PTree
  modelPTreeDot
      << "digraph G {\n"
      << "\tsize=\"10,7.5\";\n"
      << "\tratio=fill;\n"
      << "\trotate=90;\n"
      << "\tcenter = \"true\";\n"
      << "\tnode [style=\"filled\",width=.1,height=.1,fontname=\"Terminus\"]\n"
      << "\tedge [arrowsize=.3]\n"
      << "\tn" << rootPNode << " [shape=diamond];\n"
      << "\tn" << rootPNode << " -> n" << esParentPNode << " [label=0b011];\n"
      << "\tn" << rootPNode << " -> n" << rightLeafPNode << " [label=0b000];\n"
      << "\tn" << rightLeafPNode << " [shape=diamond,fillcolor=green];\n"
      << "\tn" << esParentPNode << " [shape=diamond];\n"
      << "\tn" << esParentPNode << " -> n" << es1LeafPNode
      << " [label=0b010];\n"
      << "\tn" << esParentPNode << " -> n" << esLeafPNode << " [label=0b001];\n"
      << "\tn" << esLeafPNode << " [shape=diamond,fillcolor=green];\n"
      << "\tn" << es1LeafPNode << " [shape=diamond,fillcolor=green];\n"
      << "}\n";
  std::string pTreeDot;
  llvm::raw_string_ostream pTreeDotStream(pTreeDot);
  processForest.dump(pTreeDotStream);
  EXPECT_EQ(modelPTreeDot.str(), pTreeDotStream.str());

  rp.update(&es, {&es1}, {&es});
  rp1.update(nullptr, {&es}, {&es1});

  rp1.update(&es, {}, {&es});
  processForest.remove(es.ptreeNode);

  modelPTreeDot.str("");
  modelPTreeDot
      << "digraph G {\n"
      << "\tsize=\"10,7.5\";\n"
      << "\tratio=fill;\n"
      << "\trotate=90;\n"
      << "\tcenter = \"true\";\n"
      << "\tnode [style=\"filled\",width=.1,height=.1,fontname=\"Terminus\"]\n"
      << "\tedge [arrowsize=.3]\n"
      << "\tn" << rootPNode << " [shape=diamond];\n"
      << "\tn" << rootPNode << " -> n" << esParentPNode << " [label=0b001];\n"
      << "\tn" << rootPNode << " -> n" << rightLeafPNode << " [label=0b000];\n"
      << "\tn" << rightLeafPNode << " [shape=diamond,fillcolor=green];\n"
      << "\tn" << esParentPNode << " [shape=diamond];\n"
      << "\tn" << esParentPNode << " -> n" << es1LeafPNode
      << " [label=0b001];\n"
      << "\tn" << es1LeafPNode << " [shape=diamond,fillcolor=green];\n"
      << "}\n";

  pTreeDot = "";
  processForest.dump(pTreeDotStream);
  EXPECT_EQ(modelPTreeDot.str(), pTreeDotStream.str());
  processForest.remove(es1.ptreeNode);
  processForest.remove(root.ptreeNode);
}
TEST(SearcherDeathTest, TooManyRandomPaths) {
  // First state
  ExecutionState es;
  PForest processForest = PForest();
  processForest.addRoot(&es);
  es.ptreeNode = processForest.getPTrees()
                     .at(es.ptreeNode->getTreeID())
                     ->root.getPointer();
  processForest.remove(es.ptreeNode); // Need to remove to avoid leaks

  RNG rng;
  RandomPathSearcher rp(processForest, rng);
  RandomPathSearcher rp1(processForest, rng);
  RandomPathSearcher rp2(processForest, rng);
  ASSERT_DEATH({ RandomPathSearcher rp3(processForest, rng); }, "");
}
} // namespace
