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
#include "../../lib/Core/Thread.h"
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

/// @brief ExecutionState representing a path under exploration
class ExecutionState {
public:
  typedef std::map<Thread::thread_id_t, Thread> threads_ty;
  typedef std::map<Thread::wlist_id_t, std::set<Thread::thread_id_t> > wlists_ty;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState &);

  std::map<std::string, std::string> fnAliases;

public:
  // Execution - Control Flow specific

  /// @brief Pointer to instruction to be executed after the current
  /// instruction
  KInstIterator& pc() { return crtThread().pc; }
  const KInstIterator& pc() const { return crtThread().pc; }

  /// @brief Pointer to instruction which is currently executed
  KInstIterator& prevPC() { return crtThread().prevPC; }
  const KInstIterator& prevPC() const { return crtThread().prevPC; }


  /// @brief Stack representing the current instruction stream
  Thread::stack_ty& stack() { return crtThread().stack; }
  const Thread::stack_ty& stack() const { return crtThread().stack; }

  /// @brief Remember from which Basic Block control flow arrived
  /// (i.e. to select the right phi values)
  unsigned incomingBBIndex() { return crtThread().incomingBBIndex; };
  void incomingBBIndex(unsigned ibbi) { crtThread().incomingBBIndex = ibbi; }

  // Overall state of the state - Data specific

  /// @brief Address space used by this state (e.g. Global and Heap)
  AddressSpace addressSpace;

  /// @brief Constraints collected so far
  ConstraintManager constraints;

  /// Statistics and information

  /// @brief Costs for all queries issued for this state, in seconds
  mutable double queryCost;

  /// @brief Weight assigned for importance of this state.  Can be
  /// used for searchers to decide what paths to explore
  double weight;

  /// @brief Exploration depth, i.e., number of times KLEE branched for this state
  unsigned depth;

  /// @brief History of complete path: represents branches taken to
  /// reach/create this state (both concrete and symbolic)
  TreeOStream pathOS;

  /// @brief History of symbolic path: represents symbolic branches
  /// taken to reach/create this state
  TreeOStream symPathOS;

  /// @brief Counts how many instructions were executed since the last new
  /// instruction was covered.
  unsigned instsSinceCovNew;

  /// @brief Whether a new instruction was covered in this state
  bool coveredNew;

  /// @brief Disables forking for this state. Set by user code
  bool forkDisabled;

  /// @brief Set containing which lines in which files are covered by this state
  std::map<const std::string *, std::set<unsigned> > coveredLines;

  /// @brief Pointer to the process tree of the current state
  PTreeNode *ptreeNode;

  /// @brief Ordered list of symbolics: used to generate test cases.
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector<std::pair<const MemoryObject *, const Array *> > symbolics;

  /// @brief Set of used array names for this state.  Used to avoid collisions.
  std::set<std::string> arrayNames;

  std::string getFnAlias(std::string fn);
  void addFnAlias(std::string old_fn, std::string new_fn);
  void removeFnAlias(std::string fn);

  // @brief Threads in current state
  threads_ty threads;

  // @brief Pointer to current thread
  threads_ty::iterator crtThreadIt;
  Thread &crtThread() { return crtThreadIt->second; }
  const Thread &crtThread() const { return crtThreadIt->second; }

  // @brief Waiting lists to block threads
  wlists_ty waitingLists;
  Thread::wlist_id_t wlistCounter;

  // @brief Accumulated preemptions
  unsigned int preemptions;

  // @brief List of context switches performed
  std::vector<Thread::thread_id_t> schedulingHistory;

  // @brief Create a new thread in the state
  Thread& createThread(Thread::thread_id_t tid, KFunction *kf);

  // @brief Terminate the specified thread
  void terminateThread(threads_ty::iterator it);

  // @brief Get next thread to be scheduled (round robin)
  threads_ty::iterator nextThread(threads_ty::iterator it) {
    if (it == threads.end())
      it = threads.begin();
    else {
      it++;
      if (it == threads.end())
        it = threads.begin();
    }
    return it;
  }

  // @brief Set thread as active thread
  void scheduleNext(threads_ty::iterator it) {
    assert(it != threads.end());
    crtThreadIt = it;
    schedulingHistory.push_back(crtThread().tid);
  }

  // @brief Generate a new waiting list
  Thread::wlist_id_t getWaitingList() { return wlistCounter++; }


  void sleepThread(Thread::wlist_id_t wlist);
  void notifyOne(Thread::wlist_id_t wlist, Thread::thread_id_t tid);
  void notifyAll(Thread::wlist_id_t wlist);

private:
  ExecutionState() : ptreeNode(0) {}

  void setupMain(KFunction *kf);
public:
  ExecutionState(KFunction *kf);

  // XXX total hack, just used to make a state so solver can
  // use on structure
  ExecutionState(const std::vector<ref<Expr> > &assumptions);

  ExecutionState(const ExecutionState &state);

  ~ExecutionState();

  ExecutionState *branch();

  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame(Thread &t);
  void popFrame();

  void addSymbolic(const MemoryObject *mo, const Array *array);
  void addConstraint(ref<Expr> e) { constraints.addConstraint(e); }

  bool merge(const ExecutionState &b);
  void dumpStack(llvm::raw_ostream &out) const;
};
}

#endif
