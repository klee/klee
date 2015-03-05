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
#include "../../lib/Core/Threading.h"
#include "klee/Internal/Module/KInstIterator.h"

#include <map>
#include <set>
#include <vector>

namespace klee {
  class Array;
  struct KFunction;
  struct KInstruction;
  class MemoryObject;
  class PTreeNode;
  struct InstructionInfo;
//  class Thread;
//  struct StackFrame;

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemoryMap &mm);

class ExecutionState {

public:
  typedef std::vector<StackFrame> stack_ty;
  typedef std::map<thread_id_t, Thread> threads_ty;
  typedef std::map<wlist_id_t, std::set<thread_id_t> > wlists_ty;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState&); 
  std::map< std::string, std::string > fnAliases;

public:
  bool fakeState;
  unsigned depth;
  
  ConstraintManager constraints;
  mutable double queryCost;
  double weight;
  AddressSpace addressSpace;
  TreeOStream pathOS, symPathOS;
  unsigned instsSinceCovNew;
  bool coveredNew;

  /// Disables forking, set by user code.
  bool forkDisabled;

  std::map<const std::string*, std::set<unsigned> > coveredLines;
  PTreeNode *ptreeNode;

  /// ordered list of symbolics: used to generate test cases. 
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector< std::pair<const MemoryObject*, const Array*> > symbolics;

  /// Set of used array names.  Used to avoid collisions.
  std::set<std::string> arrayNames;

  // Used by the checkpoint/rollback methods for fake objects.
  // FIXME: not freeing things on branch deletion.
  MemoryMap shadowObjects;

  std::string getFnAlias(std::string fn);
  void addFnAlias(std::string old_fn, std::string new_fn);
  void removeFnAlias(std::string fn);

  // Thread scheduling
  // For a multi threaded ExecutionState
  threads_ty threads;

  wlists_ty waitingLists;
  wlist_id_t wlistCounter;

  Thread& createThread(thread_id_t tid, KFunction *kf);
  void terminateThread(threads_ty::iterator it);

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

  void scheduleNext(threads_ty::iterator it) {
      assert(it != threads.end());
      crtThreadIt = it;
  }

  wlist_id_t getWaitingList() { return wlistCounter++; }
  void sleepThread(wlist_id_t wlist);
  void notifyOne(wlist_id_t wlist, thread_id_t tid);
  void notifyAll(wlist_id_t wlist);

  threads_ty::iterator crtThreadIt;

  unsigned int preemptions;

  /* Shortcut methods */

  Thread &crtThread() { return crtThreadIt->second; }
  const Thread &crtThread() const { return crtThreadIt->second; }

  KInstIterator& pc() { return crtThread().pc; }
  const KInstIterator& pc() const { return crtThread().pc; }
  
  void pc(KInstIterator& ki) { crtThread().pc = ki; }
  void pc(const KInstIterator& ki) { crtThread().pc = ki; }
  
  KInstIterator& prevPC() { return crtThread().prevPC; }
  const KInstIterator& prevPC() const { return crtThread().prevPC; }
  
  void prevPC(KInstIterator& ki) { crtThread().prevPC = ki; }
  void prevPC(const KInstIterator& ki) { crtThread().prevPC = ki; }
 
  stack_ty& stack() { return crtThread().stack; }
  const stack_ty& stack() const { return crtThread().stack; }
  
  void stack(stack_ty& s) { crtThread().stack = s; }
  void stack(const stack_ty& s) { crtThread().stack = s; }
  
  unsigned incomingBBIndex() { return crtThread().incomingBBIndex; }
  
  void incomingBBIndex(unsigned ibbi) { crtThread().incomingBBIndex = ibbi; }
  
private:
  ExecutionState() : fakeState(false), ptreeNode(0) {}

  void setupMain(KFunction *kf);
public:
  ExecutionState(KFunction *kf);

  // XXX total hack, just used to make a state so solver can
  // use on structure
  ExecutionState(const std::vector<ref<Expr> > &assumptions);

  ExecutionState(const ExecutionState& state);

  ~ExecutionState();
  
  ExecutionState *branch();

  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame();

  void addSymbolic(const MemoryObject *mo, const Array *array);
  void addConstraint(ref<Expr> e) { 
    constraints.addConstraint(e); 
  }

  bool merge(const ExecutionState &b);
  void dumpStack(llvm::raw_ostream &out) const;
};

}

#endif
