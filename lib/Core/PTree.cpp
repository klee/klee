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
  root = new PTreeNode(nullptr, initialState);
  initialState->ptreeNode = root;
}

void PTree::attach(PTreeNode *node, ExecutionState *leftState, ExecutionState *rightState) {
  assert(node && !node->left && !node->right);

  node->state = nullptr;
  node->left = new PTreeNode(node, leftState);
  node->right = new PTreeNode(node, rightState);
}

void PTree::remove(PTreeNode *n) {
  assert(!n->left && !n->right);
  do {
    PTreeNode *p = n->parent;
    if (p) {
      if (n == p->left) {
        p->left = nullptr;
      } else {
        assert(n == p->right);
        p->right = nullptr;
      }
    }
    delete n;
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
  stack.push_back(root);
  while (!stack.empty()) {
    const PTreeNode *n = stack.back();
    stack.pop_back();
    os << "\tn" << n << " [shape=diamond";
    if (n->state)
      os << ",fillcolor=green";
    os << "];\n";
    if (n->left) {
      os << "\tn" << n << " -> n" << n->left << ";\n";
      stack.push_back(n->left);
    }
    if (n->right) {
      os << "\tn" << n << " -> n" << n->right << ";\n";
      stack.push_back(n->right);
    }
  }
  os << "}\n";
  delete pp;
}

PTreeNode::PTreeNode(PTreeNode *parent, ExecutionState *state) : parent{parent}, state{state} {
  state->ptreeNode = this;
}

