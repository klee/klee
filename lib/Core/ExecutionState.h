//===-- ExecutionState.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTIONSTATE_H
#define KLEE_EXECUTIONSTATE_H

#include "AddressSpace.h"
#include "MergeHandler.h"

#include "klee/ADT/TreeStream.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KInstIterator.h"
#include "klee/Solver/Solver.h"
#include "klee/System/Time.h"

#include <map>
#include <set>
#include <vector>

namespace klee {
class Array;
class CallPathNode;
struct Cell;
struct KFunction;
struct KInstruction;
class MemoryObject;
class PTreeNode;
struct InstructionInfo;

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemoryMap &mm);

struct StackFrame {
  KInstIterator caller;
  KFunction *kf;
  CallPathNode *callPathNode;

  std::vector<const MemoryObject *> allocas;
  Cell *locals;

  /// Minimum distance to an uncovered instruction once the function
  /// returns. This is not a good place for this but is used to
  /// quickly compute the context sensitive minimum distance to an
  /// uncovered instruction. This value is updated by the StatsTracker
  /// periodically.
  unsigned minDistToUncoveredOnReturn;

  // For vararg functions: arguments not passed via parameter are
  // stored (packed tightly) in a local (alloca) memory object. This
  // is set up to match the way the front-end generates vaarg code (it
  // does not pass vaarg through as expected). VACopy is lowered inside
  // of intrinsic lowering.
  MemoryObject *varargs;

  StackFrame(KInstIterator caller, KFunction *kf);
  StackFrame(const StackFrame &s);
  ~StackFrame();
};

/// @brief ExecutionState representing a path under exploration
class ExecutionState {
#ifdef KLEE_UNITTEST
public:
#else
private:
#endif
  // copy ctor
  ExecutionState(const ExecutionState &state);

public:
  using stack_ty = std::vector<StackFrame>;

  // Execution - Control Flow specific

  /// @brief Pointer to instruction to be executed after the current
  /// instruction
  KInstIterator pc;

  /// @brief Pointer to instruction which is currently executed
  KInstIterator prevPC;

  /// @brief Stack representing the current instruction stream
  stack_ty stack;

  /// @brief Remember from which Basic Block control flow arrived
  /// (i.e. to select the right phi values)
  std::uint32_t incomingBBIndex;

  // Overall state of the state - Data specific

  /// @brief Exploration depth, i.e., number of times KLEE branched for this state
  std::uint32_t depth;

  /// @brief Address space used by this state (e.g. Global and Heap)
  AddressSpace addressSpace;

  /// @brief Constraints collected so far
  ConstraintSet constraints;

  /// Statistics and information

  /// @brief Metadata utilized and collected by solvers for this state
  mutable SolverQueryMetaData queryMetaData;

  /// @brief History of complete path: represents branches taken to
  /// reach/create this state (both concrete and symbolic)
  TreeOStream pathOS;

  /// @brief History of symbolic path: represents symbolic branches
  /// taken to reach/create this state
  TreeOStream symPathOS;

  /// @brief Set containing which lines in which files are covered by this state
  std::map<const std::string *, std::set<std::uint32_t>> coveredLines;

  /// @brief Pointer to the process tree of the current state
  /// Copies of ExecutionState should not copy ptreeNode
  PTreeNode *ptreeNode = nullptr;

  /// @brief Ordered list of symbolics: used to generate test cases.
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector<std::pair<ref<const MemoryObject>, const Array *>> symbolics;

  /// @brief Set of used array names for this state.  Used to avoid collisions.
  std::set<std::string> arrayNames;

  /// @brief The objects handling the klee_open_merge calls this state ran through
  std::vector<ref<MergeHandler>> openMergeStack;

  /// @brief The numbers of times this state has run through Executor::stepInstruction
  std::uint64_t steppedInstructions;

  /// @brief Counts how many instructions were executed since the last new
  /// instruction was covered.
  std::uint32_t instsSinceCovNew;

  /// @brief the global state counter
  static std::uint32_t nextID;

  /// @brief the state id
  std::uint32_t id {0};

  /// @brief Whether a new instruction was covered in this state
  bool coveredNew;

  /// @brief Disables forking for this state. Set by user code
  bool forkDisabled;

public:
  #ifdef KLEE_UNITTEST
  // provide this function only in the context of unittests
  ExecutionState(){}
  #endif
  // only to create the initial state
  explicit ExecutionState(KFunction *kf);
  // no copy assignment, use copy constructor
  ExecutionState &operator=(const ExecutionState &) = delete;
  // no move ctor
  ExecutionState(ExecutionState &&) noexcept = delete;
  // no move assignment
  ExecutionState& operator=(ExecutionState &&) noexcept = delete;
  // dtor
  ~ExecutionState();

  ExecutionState *branch();

  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame();

  void addSymbolic(const MemoryObject *mo, const Array *array);

  void addConstraint(ref<Expr> e);

  bool merge(const ExecutionState &b);
  void dumpStack(llvm::raw_ostream &out) const;

  std::uint32_t getID() const { return id; };
  void setID() { id = nextID++; };
};

struct ExecutionStateIDCompare {
  bool operator()(const ExecutionState *a, const ExecutionState *b) const {
    return a->getID() < b->getID();
  }
};
}

#endif /* KLEE_EXECUTIONSTATE_H */
