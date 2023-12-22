//===-- ExecutionTree.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTION_TREE_H
#define KLEE_EXECUTION_TREE_H

#include "ExecutionTreeWriter.h"
#include "UserSearcher.h"
#include "klee/Core/BranchTypes.h"
#include "klee/Core/TerminationTypes.h"
#include "klee/Expr/Expr.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Support/Casting.h"

#include <cstdint>
#include <variant>

namespace klee {
class ExecutionState;
class Executor;
class InMemoryExecutionTree;
class InterpreterHandler;
class ExecutionTreeNode;
class Searcher;

/* ExecutionTreeNodePtr is used by the Random Path Searcher object to
efficiently record which ExecutionTreeNode belongs to it. ExecutionTree is a
global structure that captures all  states, whereas a Random Path Searcher might
only care about a subset. The integer part of ExecutionTreeNodePtr is a bitmask
(a "tag") of which Random Path Searchers ExecutionTreeNode belongs to. */
constexpr std::uint8_t PtrBitCount = 3;
using ExecutionTreeNodePtr =
    llvm::PointerIntPair<ExecutionTreeNode *, PtrBitCount, uint8_t>;

class ExecutionTreeNode {
public:
  enum class NodeType : std::uint8_t { Basic, Annotated };

  ExecutionTreeNode *parent{nullptr};
  ExecutionTreeNodePtr left;
  ExecutionTreeNodePtr right;
  ExecutionState *state{nullptr};

  ExecutionTreeNode(ExecutionTreeNode *parent, ExecutionState *state) noexcept;
  virtual ~ExecutionTreeNode() = default;
  ExecutionTreeNode(const ExecutionTreeNode &) = delete;
  ExecutionTreeNode &operator=(ExecutionTreeNode const &) = delete;
  ExecutionTreeNode(ExecutionTreeNode &&) = delete;
  ExecutionTreeNode &operator=(ExecutionTreeNode &&) = delete;

  [[nodiscard]] virtual NodeType getType() const { return NodeType::Basic; }
  static bool classof(const ExecutionTreeNode *N) { return true; }
};

class AnnotatedExecutionTreeNode : public ExecutionTreeNode {
  inline static std::uint32_t nextID{1};

public:
  std::uint32_t id{0};
  std::uint32_t stateID{0};
  std::uint32_t asmLine{0};
  std::variant<BranchType, StateTerminationType> kind{BranchType::NONE};

  AnnotatedExecutionTreeNode(ExecutionTreeNode *parent,
                             ExecutionState *state) noexcept;
  ~AnnotatedExecutionTreeNode() override = default;

  [[nodiscard]] NodeType getType() const override { return NodeType::Annotated; }

  static bool classof(const ExecutionTreeNode *N) {
    return N->getType() == NodeType::Annotated;
  }
};

class ExecutionTree {
public:
  enum class ExecutionTreeType : std::uint8_t {
    Basic,
    Noop,
    InMemory,
    Persistent
  };

  /// Branch from ExecutionTreeNode and attach states, convention: rightState is
  /// parent
  virtual void attach(ExecutionTreeNode *node, ExecutionState *leftState,
                      ExecutionState *rightState, BranchType reason) = 0;
  /// Dump execution tree in .dot format into os (debug)
  virtual void dump(llvm::raw_ostream &os) = 0;
  /// Remove node from tree
  virtual void remove(ExecutionTreeNode *node) = 0;
  /// Set termination type (on state removal)
  virtual void setTerminationType(ExecutionState &state,
                                  StateTerminationType type){}

  virtual ~ExecutionTree() = default;
  ExecutionTree(ExecutionTree const &) = delete;
  ExecutionTree &operator=(ExecutionTree const &) = delete;
  ExecutionTree(ExecutionTree &&) = delete;
  ExecutionTree &operator=(ExecutionTree &&) = delete;

  [[nodiscard]] virtual ExecutionTreeType getType() const = 0;
  static bool classof(const ExecutionTreeNode *N) { return true; }

protected:
  explicit ExecutionTree() noexcept = default;
};

/// @brief A pseudo execution tree that does not maintain any nodes.
class NoopExecutionTree final : public ExecutionTree {
public:
  NoopExecutionTree() noexcept = default;
  ~NoopExecutionTree() override = default;
  void attach(ExecutionTreeNode *node, ExecutionState *leftState,
              ExecutionState *rightState, BranchType reason) noexcept override {}
  void dump(llvm::raw_ostream &os) noexcept override;
  void remove(ExecutionTreeNode *node) noexcept override {}

  [[nodiscard]] ExecutionTreeType getType() const override {
    return ExecutionTreeType::Noop;
  };

  static bool classof(const ExecutionTree *T) {
    return T->getType() == ExecutionTreeType::Noop;
  }
};

/// @brief An in-memory execution tree required by RandomPathSearcher
class InMemoryExecutionTree : public ExecutionTree {
public:
  ExecutionTreeNodePtr root;

private:
  /// Number of registered IDs ("users", e.g. RandomPathSearcher)
  std::uint8_t registeredIds = 0;

  virtual ExecutionTreeNode *createNode(ExecutionTreeNode *parent,
                                        ExecutionState *state);
  virtual void updateBranchingNode(ExecutionTreeNode &node, BranchType reason) {}
  virtual void updateTerminatingNode(ExecutionTreeNode &node) {}

public:
  InMemoryExecutionTree() noexcept = default;
  explicit InMemoryExecutionTree(ExecutionState &initialState) noexcept;
  ~InMemoryExecutionTree() override = default;

  void attach(ExecutionTreeNode *node, ExecutionState *leftState,
              ExecutionState *rightState, BranchType reason) noexcept override;
  void dump(llvm::raw_ostream &os) noexcept override;
  std::uint8_t getNextId() noexcept;
  void remove(ExecutionTreeNode *node) noexcept override;

  [[nodiscard]] ExecutionTreeType getType() const override {
    return ExecutionTreeType::InMemory;
  };

  static bool classof(const ExecutionTree *T) {
    return (T->getType() == ExecutionTreeType::InMemory) ||
           (T->getType() == ExecutionTreeType::Persistent);
  }
};

/// @brief An in-memory execution tree that also writes its nodes into an SQLite
/// database (exec_tree.db) with a ExecutionTreeWriter
class PersistentExecutionTree : public InMemoryExecutionTree {
  ExecutionTreeWriter writer;

  ExecutionTreeNode *createNode(ExecutionTreeNode *parent,
                                ExecutionState *state) override;
  void updateBranchingNode(ExecutionTreeNode &node, BranchType reason) override;
  void updateTerminatingNode(ExecutionTreeNode &node) override;

public:
  explicit PersistentExecutionTree(ExecutionState &initialState,
                                   InterpreterHandler &ih) noexcept;
  ~PersistentExecutionTree() override = default;
  void dump(llvm::raw_ostream &os) noexcept override;
  void setTerminationType(ExecutionState &state,
                          StateTerminationType type) override;

  [[nodiscard]] ExecutionTreeType getType() const override {
    return ExecutionTreeType::Persistent;
  };

  static bool classof(const ExecutionTree *T) {
    return T->getType() == ExecutionTreeType::Persistent;
  }
};

std::unique_ptr<ExecutionTree> createExecutionTree(ExecutionState &initialState,
                                                   bool inMemory,
                                                   InterpreterHandler &ih);
} // namespace klee

#endif /* KLEE_EXECUTION_TREE_H */
