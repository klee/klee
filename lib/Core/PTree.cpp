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
#include "klee/Support/OptionCategories.h"

#include "llvm/Support/CommandLine.h"

#include <bitset>
#include <vector>

using namespace klee;
using namespace llvm;

namespace {

cl::opt<bool>
    CompressProcessTree("compress-process-tree",
                        cl::desc("Remove intermediate nodes in the process "
                                 "tree whenever possible (default=false)"),
                        cl::init(false), cl::cat(MiscCat));

} // namespace

unsigned CHUNK_IDX = 1;
// Sizes of a full binary tree. We can't index more than 8 in the bit stolen
// pointers.
constexpr uint8_t CHUNK_SIZES[8] = {1, 3, 7, 15, 31, 63, 127, 255};
// constexpr int DEFAULT_CHUNK_IDX = 1;
#define NUM_SPACES (CHUNK_SIZES[CHUNK_IDX])

/* PTreeNodes are layed out in a cache friendly manner via van Embde Boas layout
   as described in https://jiahai-feng.github.io/posts/cache-oblivious-algorithms/
   and http://erikdemaine.org/papers/BRICS2002/paper.pdf (Figure 4).

   The idea is to partion the tree recursivelly into small subtrees and then have
   the little subtrees allocated togheter.  This is achived in two steps:

   1) PTreeNodes is allocated in a chunk of memory that is capable of holding a full
      subtree of PTreeNodes. That is 1, 3, 7, 15 ... nodes. CHUNK_SIZES[CHUNK_IDX] 
      determines this size
   2) When allocating a new PTreeNode, we check if the parent has any space left in 
      its chunk and allocate there. Otherwise go to 1).
   */

PTree::PTree(ExecutionState *initialState) {
  if(CompressProcessTree) CHUNK_IDX = 0; 
  uint8_t *chunk = new uint8_t[(NUM_SPACES * sizeof(PTreeNode))];
  root = PTreeNodePtr(new (chunk) PTreeNode(nullptr, CHUNK_IDX, initialState));
  initialState->ptreeNode = root.getPointer();
}

void PTree::attach(PTreeNode *node, ExecutionState *leftState, ExecutionState *rightState) {
  assert(node && !node->left.getPointer() && !node->right.getPointer());
  assert(node == rightState->ptreeNode &&
         "Attach assumes the right state is the current state");

  uint8_t *left_chunk, *right_chunk;
  auto space_left_idx = node->parent.getInt();
  if (space_left_idx > 0) {
    auto space_left = CHUNK_SIZES[space_left_idx];
    left_chunk = (uint8_t *)node + sizeof(PTreeNode);
    right_chunk =
        (uint8_t *)node + (((space_left / 2) + 1) * sizeof(PTreeNode));
    space_left_idx--;
  } else {
    left_chunk = new uint8_t[(NUM_SPACES * sizeof(PTreeNode))];
    right_chunk = new uint8_t[(NUM_SPACES * sizeof(PTreeNode))];
    space_left_idx = CHUNK_IDX;
  }

  node->state = nullptr;
  node->left =
      PTreeNodePtr(new (left_chunk) PTreeNode(node, space_left_idx, leftState));
  // The current node inherits the tag
  uint8_t currentNodeTag = root.getInt();
  if (node->parent.getPointer())
    currentNodeTag = node->parent.getPointer()->left.getPointer() == node
                         ? node->parent.getPointer()->left.getInt()
                         : node->parent.getPointer()->right.getInt();
  node->right = PTreeNodePtr(new (right_chunk)
                                 PTreeNode(node, space_left_idx, rightState),
                             currentNodeTag);
}

void PTree::remove(PTreeNode *n) {
  assert(!n->left.getPointer() && !n->right.getPointer());
  do {
    PTreeNode *p = n->parent.getPointer();
    if (p) {
      if (n == p->left.getPointer()) {
        p->left = PTreeNodePtr(nullptr);
      } else {
        assert(n == p->right.getPointer());
        p->right = PTreeNodePtr(nullptr);
      }
    }
    if (n->parent.getInt() == CHUNK_IDX) {
      // We are at the parent chunk and can free memory
      n->~PTreeNode();
      delete[] n;
    } else { 
      // We are in the middle of a chunk, can only destruct
      n->~PTreeNode();
    }
    n = p;
  } while (n && !n->left.getPointer() && !n->right.getPointer());

  if (n && CompressProcessTree) {
    // We're now at a node that has exactly one child; we've just deleted the
    // other one. Eliminate the node and connect its child to the parent
    // directly (if it's not the root).
    PTreeNodePtr child = n->left.getPointer() ? n->left : n->right;
    PTreeNodePtr parent = n->parent;

    child.getPointer()->parent = parent;
    if (!parent.getPointer()) {
      // We're at the root.
      root = child;
    } else {
      if (n == parent.getPointer()->left.getPointer()) {
        parent.getPointer()->left = child;
      } else {
        assert(n == parent.getPointer()->right.getPointer());
        parent.getPointer()->right = child;
      }
    }

    delete[] n;
  }
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
  stack.push_back(root.getPointer());
  while (!stack.empty()) {
    const PTreeNode *n = stack.back();
    stack.pop_back();
    os << "\tn" << n << " [shape=diamond";
    if (n->state)
      os << ",fillcolor=green";
    os << "];\n";
    if (n->left.getPointer()) {
      os << "\tn" << n << " -> n" << n->left.getPointer();
      os << " [label=0b"
         << std::bitset<PtrBitCount>(n->left.getInt()).to_string() << "];\n";
      stack.push_back(n->left.getPointer());
    }
    if (n->right.getPointer()) {
      os << "\tn" << n << " -> n" << n->right.getPointer();
      os << " [label=0b"
         << std::bitset<PtrBitCount>(n->right.getInt()).to_string() << "];\n";
      stack.push_back(n->right.getPointer());
    }
  }
  os << "}\n";
  delete pp;
}

PTreeNode::PTreeNode(PTreeNode *parent, uint8_t numSpaces,
                     ExecutionState *state)
    : parent{parent, numSpaces}, state{state} {
  state->ptreeNode = this;
  left = PTreeNodePtr(nullptr);
  right = PTreeNodePtr(nullptr);
}
PTreeNode::PTreeNode(PTreeNode *parent, ExecutionState *state) : parent{parent}, state{state} {
  state->ptreeNode = this;
  left = PTreeNodePtr(nullptr);
  right = PTreeNodePtr(nullptr);
}
