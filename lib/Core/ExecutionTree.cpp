//===-- ExecutionTree.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ExecutionTree.h"

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
    ExecTreeCat("Execution tree related options",
                "These options affect the execution tree handling.");
}

namespace {
llvm::cl::opt<bool> CompressExecutionTree(
    "compress-exec-tree",
    llvm::cl::desc("Remove intermediate nodes in the execution "
                   "tree whenever possible (default=false)"),
    llvm::cl::init(false), llvm::cl::cat(ExecTreeCat));

llvm::cl::opt<bool> WriteExecutionTree(
    "write-exec-tree", llvm::cl::init(false),
    llvm::cl::desc("Write execution tree into exec_tree.db (default=false)"),
    llvm::cl::cat(ExecTreeCat));
} // namespace

// ExecutionTreeNode

ExecutionTreeNode::ExecutionTreeNode(ExecutionTreeNode *parent,
                                     ExecutionState *state) noexcept
    : parent{parent}, left{nullptr}, right{nullptr}, state{state} {
  state->executionTreeNode = this;
}

// AnnotatedExecutionTreeNode

AnnotatedExecutionTreeNode::AnnotatedExecutionTreeNode(
    ExecutionTreeNode *parent, ExecutionState *state) noexcept
    : ExecutionTreeNode(parent, state) {
  id = nextID++;
}

// NoopExecutionTree

void NoopExecutionTree::dump(llvm::raw_ostream &os) noexcept {
  os << "digraph G {\nTreeNotAvailable [shape=box]\n}";
}

// InMemoryExecutionTree

InMemoryExecutionTree::InMemoryExecutionTree(
    ExecutionState &initialState) noexcept {
  root = ExecutionTreeNodePtr(createNode(nullptr, &initialState));
  initialState.executionTreeNode = root.getPointer();
}

ExecutionTreeNode *InMemoryExecutionTree::createNode(ExecutionTreeNode *parent,
                                                     ExecutionState *state) {
  return new ExecutionTreeNode(parent, state);
}

void InMemoryExecutionTree::attach(ExecutionTreeNode *node,
                                   ExecutionState *leftState,
                                   ExecutionState *rightState,
                                   BranchType reason) noexcept {
  assert(node && !node->left.getPointer() && !node->right.getPointer());
  assert(node == rightState->executionTreeNode &&
         "Attach assumes the right state is the current state");
  node->left = ExecutionTreeNodePtr(createNode(node, leftState));
  // The current node inherits the tag
  uint8_t currentNodeTag = root.getInt();
  if (node->parent)
    currentNodeTag = node->parent->left.getPointer() == node
                         ? node->parent->left.getInt()
                         : node->parent->right.getInt();
  node->right =
      ExecutionTreeNodePtr(createNode(node, rightState), currentNodeTag);
  updateBranchingNode(*node, reason);
  node->state = nullptr;
}

void InMemoryExecutionTree::remove(ExecutionTreeNode *n) noexcept {
  assert(!n->left.getPointer() && !n->right.getPointer());
  updateTerminatingNode(*n);
  do {
    ExecutionTreeNode *p = n->parent;
    if (p) {
      if (n == p->left.getPointer()) {
        p->left = ExecutionTreeNodePtr(nullptr);
      } else {
        assert(n == p->right.getPointer());
        p->right = ExecutionTreeNodePtr(nullptr);
      }
    }
    delete n;
    n = p;
  } while (n && !n->left.getPointer() && !n->right.getPointer());

  if (n && CompressExecutionTree) {
    // We are now at a node that has exactly one child; we've just deleted the
    // other one. Eliminate the node and connect its child to the parent
    // directly (if it's not the root).
    ExecutionTreeNodePtr child = n->left.getPointer() ? n->left : n->right;
    ExecutionTreeNode *parent = n->parent;

    child.getPointer()->parent = parent;
    if (!parent) {
      // We are at the root
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

void InMemoryExecutionTree::dump(llvm::raw_ostream &os) noexcept {
  std::unique_ptr<ExprPPrinter> pp(ExprPPrinter::create(os));
  pp->setNewline("\\l");
  os << "digraph G {\n"
     << "\tsize=\"10,7.5\";\n"
     << "\tratio=fill;\n"
     << "\trotate=90;\n"
     << "\tcenter = \"true\";\n"
     << "\tnode [style=\"filled\",width=.1,height=.1,fontname=\"Terminus\"]\n"
     << "\tedge [arrowsize=.3]\n";
  std::vector<const ExecutionTreeNode *> stack;
  stack.push_back(root.getPointer());
  while (!stack.empty()) {
    const ExecutionTreeNode *n = stack.back();
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

std::uint8_t InMemoryExecutionTree::getNextId() noexcept {
  static_assert(PtrBitCount <= 8);
  std::uint8_t id = 1 << registeredIds++;
  if (registeredIds > PtrBitCount) {
    klee_error("ExecutionTree cannot support more than %d RandomPathSearchers",
               PtrBitCount);
  }
  return id;
}

// PersistentExecutionTree

PersistentExecutionTree::PersistentExecutionTree(
    ExecutionState &initialState, InterpreterHandler &ih) noexcept
    : writer(ih.getOutputFilename("exec_tree.db")) {
  root = ExecutionTreeNodePtr(createNode(nullptr, &initialState));
  initialState.executionTreeNode = root.getPointer();
}

void PersistentExecutionTree::dump(llvm::raw_ostream &os) noexcept {
  writer.batchCommit(true);
  InMemoryExecutionTree::dump(os);
}

ExecutionTreeNode *
PersistentExecutionTree::createNode(ExecutionTreeNode *parent,
                                    ExecutionState *state) {
  return new AnnotatedExecutionTreeNode(parent, state);
}

void PersistentExecutionTree::setTerminationType(ExecutionState &state,
                                                 StateTerminationType type) {
  auto *annotatedNode =
      llvm::cast<AnnotatedExecutionTreeNode>(state.executionTreeNode);
  annotatedNode->kind = type;
}

void PersistentExecutionTree::updateBranchingNode(ExecutionTreeNode &node,
                                                  BranchType reason) {
  auto *annotatedNode = llvm::cast<AnnotatedExecutionTreeNode>(&node);
  const auto &state = *node.state;
  const auto prevPC = state.prevPC;
  annotatedNode->asmLine =
      prevPC && prevPC->info ? prevPC->info->assemblyLine : 0;
  annotatedNode->kind = reason;
  writer.write(*annotatedNode);
}

void PersistentExecutionTree::updateTerminatingNode(ExecutionTreeNode &node) {
  assert(node.state);
  auto *annotatedNode = llvm::cast<AnnotatedExecutionTreeNode>(&node);
  const auto &state = *node.state;
  const auto prevPC = state.prevPC;
  annotatedNode->asmLine =
      prevPC && prevPC->info ? prevPC->info->assemblyLine : 0;
  annotatedNode->stateID = state.getID();
  writer.write(*annotatedNode);
}

// Factory

std::unique_ptr<ExecutionTree>
klee::createExecutionTree(ExecutionState &initialState, bool inMemory,
                          InterpreterHandler &ih) {
  if (WriteExecutionTree)
    return std::make_unique<PersistentExecutionTree>(initialState, ih);

  if (inMemory)
    return std::make_unique<InMemoryExecutionTree>(initialState);

  return std::make_unique<NoopExecutionTree>();
};
