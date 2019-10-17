//===-- PTree.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PTree.h"

#include "ExecutionState.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"

#include <vector>

using namespace klee;

PTree::PTree(ExecutionState *initialState) {
  root = new PTreeNode(nullptr, initialState);
  initialState->ptreeNode = root;
}

void PTree::attach(PTreeNode *node, ExecutionState *leftState, ExecutionState *rightState) {
  assert(node && !node->left.getPointer() && !node->right.getPointer());
  node->state = nullptr;
  node->left = PTreeNodePtr(new PTreeNode(node, leftState));
  node->right = PTreeNodePtr(new PTreeNode(node, rightState));
}

void PTree::remove(PTreeNode *n) {
  assert(!n->left.getPointer() && !n->right.getPointer());
  do {
    PTreeNode *p = n->parent;
    if (p) {
      if (n == p->left.getPointer()) {
        p->left = PTreeNodePtr(nullptr);
      } else {
        assert(n == p->right.getPointer());
        p->right = PTreeNodePtr(nullptr);
      }
    }
    delete n;
    n = p;
  } while (n && !n->left.getPointer() && !n->right.getPointer());
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
    if (n->left.getPointer()) {
      os << "\tn" << n << " -> n" << n->left.getPointer() << ";\n";
      stack.push_back(n->left.getPointer());
    }
    if (n->right.getPointer()) {
      os << "\tn" << n << " -> n" << n->right.getPointer() << ";\n";
      stack.push_back(n->right.getPointer());
    }
  }
  os << "}\n";
  delete pp;
}

PTreeNode::PTreeNode(PTreeNode *parent, ExecutionState *state) : parent{parent}, state{state} {
  state->ptreeNode = this;
  left = PTreeNodePtr(nullptr);
  right = PTreeNodePtr(nullptr);
}
