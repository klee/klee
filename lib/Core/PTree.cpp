//===-- PTree.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PTree.h"

#include <klee/Expr.h>
#include <klee/util/ExprPPrinter.h>

#include <vector>

using namespace klee;

  /* *** */

PTree::PTree(const data_type &root) : root(new Node(nullptr, root)) {}

std::pair<PTreeNode*, PTreeNode*>
PTree::split(Node *n, 
             const data_type &leftData, 
             const data_type &rightData) {
  assert(n && !n->left && !n->right);
  n->left = new Node(n, leftData);
  n->right = new Node(n, rightData);
  return std::make_pair(n->left, n->right);
}

void PTree::remove(Node *n) {
  assert(!n->left && !n->right);
  do {
    Node *p = n->parent;
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
  std::vector<PTree::Node*> stack;
  stack.push_back(root);
  while (!stack.empty()) {
    PTree::Node *n = stack.back();
    stack.pop_back();
    os << "\tn" << n << " [shape=diamond";
    if (n->data)
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

PTreeNode::PTreeNode(PTreeNode * parent, ExecutionState * data)
  : parent{parent}, data{data} {}

