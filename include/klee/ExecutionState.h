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

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/Internal/ADT/TreeStream.h"

// FIXME: We do not want to be exposing these? :(
#include "../../lib/Core/AddressSpace.h"
#include "klee/Internal/Module/KInstIterator.h"

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
  // is setup to match the way the front-end generates vaarg code (it
  // does not pass vaarg through as expected). VACopy is lowered inside
  // of intrinsic lowering.
  MemoryObject *varargs;

  StackFrame(KInstIterator caller, KFunction *kf);
  StackFrame(const StackFrame &s);
  ~StackFrame();
};

/// @brief ExecutionState representing the one state of execution
class ExecutionState {
public:
  typedef std::vector<StackFrame> stack_ty;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState &);

  std::map<std::string, std::string> fnAliases;

public:
  // Are we currently underconstrained?  Hack: value is size to make fake
  // objects.
  unsigned underConstrained;

  /// Execution - Control Flow specific

  /// @brief pointer to instruction to be executed next after the current
  /// instruction (prevPc)
  KInstIterator pc;

  /// @brief Pointer to instruction which is currently executed
  KInstIterator prevPC;

  /// @brief Stack representing the current instruction stream
  stack_ty stack;

  /// @brief Remember from which Basic Block control flow arrived (i.e. to
  /// select the right phi values)
  unsigned incomingBBIndex;

  /// Overall state of the state - Data specific

  /// @brief Address space used by this state (e.g. Global and Heap).
  AddressSpace addressSpace;

  /// @brief Constraints collected so far
  ConstraintManager constraints;

  /// Statistics and information

  /// @brief Costs for all queries in seconds for this state
  mutable double queryCost;

  /// @brief Weight assigned for importance of this state. Can be used for
  /// searchers to decide
  double weight;

  /// @brief Counter how often KLEE branched for this state
  unsigned depth;

  /// @brief History of complete path: Represents branches taken to reach/create
  /// this state
  ///        Includes symbolic-based branches as well.
  TreeOStream pathOS;

  /// @brief History of symbolic path: Represents branches taken but only
  /// symbolic ones.
  TreeOStream symPathOS;

  /// @brief Does not represent a real state just for patching purposes
  bool fakeState;

  /// @brief Counts how many instructions were executed since the last new
  /// instruction was covered.
  unsigned instsSinceCovNew;

  /// @brief Flag if new instruction was covered or not in this state
  bool coveredNew;

  /// @brief Disables forking for this state. Set by user code.
  bool forkDisabled;

  /// @brief Set containing which lines in which files are covered by this state
  std::map<const std::string *, std::set<unsigned> > coveredLines;

  /// @brief Representing pointer to process tree of current state
  PTreeNode *ptreeNode;

  /// ordered list of symbolics: used to generate test cases.
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector<std::pair<const MemoryObject *, const Array *> > symbolics;

  /// @brief Set of used array names for this state.  Used to avoid collisions.
  std::set<std::string> arrayNames;

  std::string getFnAlias(std::string fn);
  void addFnAlias(std::string old_fn, std::string new_fn);
  void removeFnAlias(std::string fn);

private:
  ExecutionState() : underConstrained(0), fakeState(false), ptreeNode(0) {}

public:
  ExecutionState(KFunction *kf);

  // XXX total hack, just used to make a state so solver can
  // use on structure
  ExecutionState(const std::vector<ref<Expr> > &assumptions);

  ExecutionState(const ExecutionState &state);

  ~ExecutionState();

  ExecutionState *branch();

  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame();

  void addSymbolic(const MemoryObject *mo, const Array *array);
  void addConstraint(ref<Expr> e) { constraints.addConstraint(e); }

  bool merge(const ExecutionState &b);
  void dumpStack(llvm::raw_ostream &out) const;
};
}

#endif
