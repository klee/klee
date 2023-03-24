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

#include "klee/Core/Interpreter.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Module/KInstruction.h"
#include "klee/Support/OptionCategories.h"

#include <bitset>
#include <vector>

using namespace klee;

namespace klee {
llvm::cl::OptionCategory
    PTreeCat("Process tree related options",
             "These options affect the process tree handling.");
}

namespace {
llvm::cl::opt<bool> CompressProcessTree(
    "compress-process-tree",
    llvm::cl::desc("Remove intermediate nodes in the process "
                   "tree whenever possible (default=false)"),
    llvm::cl::init(false), llvm::cl::cat(PTreeCat));

llvm::cl::opt<bool> WritePTree(
    "write-ptree", llvm::cl::init(false),
    llvm::cl::desc("Write process tree into ptree.db (default=false)"),
    llvm::cl::cat(PTreeCat));
} // namespace

// PTreeNode

PTreeNode::PTreeNode(PTreeNode *parent, ExecutionState *state) noexcept
    : parent{parent}, left{nullptr}, right{nullptr}, state{state} {
  state->ptreeNode = this;
}

// AnnotatedPTreeNode

AnnotatedPTreeNode::AnnotatedPTreeNode(PTreeNode *parent,
                                       ExecutionState *state) noexcept
    : PTreeNode(parent, state) {
  id = nextID++;
}

// NoopPTree

void NoopPTree::dump(llvm::raw_ostream &os) noexcept {
  os << "digraph G {\nTreeNotAvailable [shape=box]\n}";
}

// InMemoryPTree

InMemoryPTree::InMemoryPTree(ExecutionState &initialState) noexcept {
  root = PTreeNodePtr(createNode(nullptr, &initialState));
  initialState.ptreeNode = root.getPointer();
}

PTreeNode *InMemoryPTree::createNode(PTreeNode *parent, ExecutionState *state) {
  return new PTreeNode(parent, state);
}

void InMemoryPTree::attach(PTreeNode *node, ExecutionState *leftState,
                           ExecutionState *rightState,
                           BranchType reason) noexcept {
  assert(node && !node->left.getPointer() && !node->right.getPointer());
  assert(node == rightState->ptreeNode &&
         "Attach assumes the right state is the current state");
  node->left = PTreeNodePtr(createNode(node, leftState));
  // The current node inherits the tag
  uint8_t currentNodeTag = root.getInt();
  if (node->parent)
    currentNodeTag = node->parent->left.getPointer() == node
                         ? node->parent->left.getInt()
                         : node->parent->right.getInt();
  node->right = PTreeNodePtr(createNode(node, rightState), currentNodeTag);
  updateBranchingNode(*node, reason);
  node->state = nullptr;
}

void InMemoryPTree::remove(PTreeNode *n) noexcept {
  assert(!n->left.getPointer() && !n->right.getPointer());
  updateTerminatingNode(*n);
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

  if (n && CompressProcessTree) {
    // We're now at a node that has exactly one child; we've just deleted the
    // other one. Eliminate the node and connect its child to the parent
    // directly (if it's not the root).
    PTreeNodePtr child = n->left.getPointer() ? n->left : n->right;
    PTreeNode *parent = n->parent;

    child.getPointer()->parent = parent;
    if (!parent) {
      // We're at the root.
      root = child;
    } else {
      if (n == parent->left.getPointer()) {
        parent->left = child;
      } else {
        assert(n == parent->right.getPointer());
        parent->right = child;
      }
    }

    delete n;
  }
}

void InMemoryPTree::dump(llvm::raw_ostream &os) noexcept {
  std::unique_ptr<ExprPPrinter> pp(ExprPPrinter::create(os));
  pp->setNewline("\\l");
  os << "digraph G {\n"
     << "\tsize=\"10,7.5\";\n"
     << "\tratio=fill;\n"
     << "\trotate=90;\n"
     << "\tcenter = \"true\";\n"
     << "\tnode [style=\"filled\",width=.1,height=.1,fontname=\"Terminus\"]\n"
     << "\tedge [arrowsize=.3]\n";
  std::vector<const PTreeNode *> stack;
  stack.push_back(root.getPointer());
  while (!stack.empty()) {
    const PTreeNode *n = stack.back();
    stack.pop_back();
    os << "\tn" << n << " [shape=diamond";
    if (n->state)
      os << ",fillcolor=green";
    os << "];\n";
    if (n->left.getPointer()) {
      os << "\tn" << n << " -> n" << n->left.getPointer() << " [label=0b"
         << std::bitset<PtrBitCount>(n->left.getInt()).to_string() << "];\n";
      stack.push_back(n->left.getPointer());
    }
    if (n->right.getPointer()) {
      os << "\tn" << n << " -> n" << n->right.getPointer() << " [label=0b"
         << std::bitset<PtrBitCount>(n->right.getInt()).to_string() << "];\n";
      stack.push_back(n->right.getPointer());
    }
  }
  os << "}\n";
}

std::uint8_t InMemoryPTree::getNextId() noexcept {
  static_assert(PtrBitCount <= 8);
  std::uint8_t id = 1 << registeredIds++;
  if (registeredIds > PtrBitCount) {
    klee_error("PTree cannot support more than %d RandomPathSearchers",
               PtrBitCount);
  }
  return id;
}

// PersistentPTree

PersistentPTree::PersistentPTree(ExecutionState &initialState,
                                 InterpreterHandler &ih) noexcept
    : writer(ih.getOutputFilename("ptree.db")) {
  root = PTreeNodePtr(createNode(nullptr, &initialState));
  initialState.ptreeNode = root.getPointer();
}

void PersistentPTree::dump(llvm::raw_ostream &os) noexcept {
  writer.batchCommit(true);
  InMemoryPTree::dump(os);
}

PTreeNode *PersistentPTree::createNode(PTreeNode *parent,
                                       ExecutionState *state) {
  return new AnnotatedPTreeNode(parent, state);
}

void PersistentPTree::setTerminationType(ExecutionState &state,
                                         StateTerminationType type) {
  auto *annotatedNode = llvm::cast<AnnotatedPTreeNode>(state.ptreeNode);
  annotatedNode->kind = type;
}

void PersistentPTree::updateBranchingNode(PTreeNode &node, BranchType reason) {
  auto *annotatedNode = llvm::cast<AnnotatedPTreeNode>(&node);
  const auto &state = *node.state;
  const auto prevPC = state.prevPC;
  annotatedNode->asmLine =
      prevPC && prevPC->info ? prevPC->info->assemblyLine : 0;
  annotatedNode->kind = reason;
  writer.write(*annotatedNode);
}

void PersistentPTree::updateTerminatingNode(PTreeNode &node) {
  assert(node.state);
  auto *annotatedNode = llvm::cast<AnnotatedPTreeNode>(&node);
  const auto &state = *node.state;
  const auto prevPC = state.prevPC;
  annotatedNode->asmLine =
      prevPC && prevPC->info ? prevPC->info->assemblyLine : 0;
  annotatedNode->stateID = state.getID();
  writer.write(*annotatedNode);
}

// Factory

std::unique_ptr<PTree> klee::createPTree(ExecutionState &initialState,
                                         bool inMemory,
                                         InterpreterHandler &ih) {
  if (WritePTree)
    return std::make_unique<PersistentPTree>(initialState, ih);

  if (inMemory)
    return std::make_unique<InMemoryPTree>(initialState);

  return std::make_unique<NoopPTree>();
};
