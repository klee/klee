//===-- PTree.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PTree.h"

#include "klee/ExecutionState.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"

#include <vector>

using namespace klee;

PTree::PTree(ExecutionState *initialState) {
  root = std::make_unique<PTreeNode>(nullptr, initialState);
}

void PTree::attach(PTreeNode *node, ExecutionState *leftState, ExecutionState *rightState) {
  assert(node && !node->left && !node->right);

  node->state = nullptr;
  node->left = std::make_unique<PTreeNode>(node, leftState);
  node->right = std::make_unique<PTreeNode>(node, rightState);
}

void PTree::remove(PTreeNode *n) {
  assert(!n->left && !n->right);
  do {
    PTreeNode *p = n->parent;
    if (p) {
      if (n == p->left.get()) {
        p->left = nullptr;
      } else {
        assert(n == p->right.get());
        p->right = nullptr;
      }
    }
    n = p;
  } while (n && !n->left && !n->right);
}

void PTree::dump(llvm::raw_ostream &os) {
  ExprPPrinter *pp = ExprPPrinter::create(os);
  pp->setNewline("\\l");
  os << "digraph G {\n";
  os << "\tsize=\"10,7.5\";\n";
  os << "\tratio=fill;\n";
  os << "\trotate=90;\n";
  os << "\tcenter = \"true\";\n";
  os << "\tnode [style=\"filled\",width=.1,height=.1,fontname=\"Terminus\"]\n";
  os << "\tedge [arrowsize=.3]\n";
  std::vector<const PTreeNode*> stack;
  stack.push_back(root.get());
  while (!stack.empty()) {
    const PTreeNode *n = stack.back();
    stack.pop_back();
    os << "\tn" << n << " [shape=diamond";
    if (n->state)
      os << ",fillcolor=green";
    os << "];\n";
    if (n->left) {
      os << "\tn" << n << " -> n" << n->left.get() << ";\n";
      stack.push_back(n->left.get());
    }
    if (n->right) {
      os << "\tn" << n << " -> n" << n->right.get() << ";\n";
      stack.push_back(n->right.get());
    }
  }
  os << "}\n";
  delete pp;
}

PTreeNode::PTreeNode(PTreeNode *parent, ExecutionState *state) : parent{parent}, state{state} {
  state->ptreeNode = this;
}

