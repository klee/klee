//===-- PTree.h -------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_PTREE_H
#define KLEE_PTREE_H

#include "klee/Core/BranchTypes.h"
#include "klee/Expr/Expr.h"
#include "klee/Support/ErrorHandling.h"
#include "llvm/ADT/PointerIntPair.h"

namespace klee {
  class ExecutionState;
  class PTreeNode;
  /* PTreeNodePtr is used by the Random Path Searcher object to efficiently
  record which PTreeNode belongs to it. PTree is a global structure that
  captures all  states, whereas a Random Path Searcher might only care about
  a subset. The integer part of PTreeNodePtr is a bitmask (a "tag") of which
  Random Path Searchers PTreeNode belongs to. */
  constexpr int PtrBitCount = 3;
  using PTreeNodePtr = llvm::PointerIntPair<PTreeNode *, PtrBitCount, uint8_t>;

  class PTreeNode {
  public:
    PTreeNode *parent = nullptr;

    PTreeNodePtr left; 
    PTreeNodePtr right;
    ExecutionState *state = nullptr;

    PTreeNode(const PTreeNode&) = delete;
    PTreeNode(PTreeNode *parent, ExecutionState *state);
    ~PTreeNode() = default;
  };

  class PTree {
    // Number of registered ID
    int registeredIds = 0;

  public:
    PTreeNodePtr root;
    explicit PTree(ExecutionState *initialState);
    ~PTree() = default;

    void attach(PTreeNode *node, ExecutionState *leftState,
                ExecutionState *rightState, BranchType reason);
    void remove(PTreeNode *node);
    void dump(llvm::raw_ostream &os);
    std::uint8_t getNextId() {
      std::uint8_t id = 1 << registeredIds++;
      if (registeredIds > PtrBitCount) {
        klee_error("PTree cannot support more than %d RandomPathSearchers",
                   PtrBitCount);
      }
      return id;
    }
  };
}

#endif /* KLEE_PTREE_H */
