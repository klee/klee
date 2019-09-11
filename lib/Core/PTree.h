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

#include "klee/Expr/Expr.h"

namespace klee {
  class ExecutionState;

  class PTreeNode {
  public:
    PTreeNode *parent = nullptr;
    std::unique_ptr<PTreeNode> left;
    std::unique_ptr<PTreeNode> right;
    ExecutionState *state = nullptr;

    PTreeNode(const PTreeNode&) = delete;
    PTreeNode(PTreeNode *parent, ExecutionState *state);
    ~PTreeNode() = default;
  };

  class PTree {
  public:
    std::unique_ptr<PTreeNode> root;
    explicit PTree(ExecutionState *initialState);
    ~PTree() = default;

    static void attach(PTreeNode *node, ExecutionState *leftState, ExecutionState *rightState);
    static void remove(PTreeNode *node);
    void dump(llvm::raw_ostream &os);
  };
}

#endif /* KLEE_PTREE_H */
