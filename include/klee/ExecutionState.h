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
  class Cell;
  class KFunction;
  class KInstruction;
  class MemoryObject;
  class PTreeNode;
  class InstructionInfo;
  class ExecutionTraceEvent;

std::ostream &operator<<(std::ostream &os, const MemoryMap &mm);

struct StackFrame {
  KInstIterator caller;
  KFunction *kf;
  CallPathNode *callPathNode;

  std::vector<const MemoryObject*> allocas;
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

// FIXME: Redo execution trace stuff to use a TreeStream, there is no
// need to keep this stuff in memory as far as I can tell.

// each state should have only one of these guys ...
class ExecutionTraceManager {
public:
  ExecutionTraceManager() : hasSeenUserMain(false) {}

  void addEvent(ExecutionTraceEvent* evt);
  void printAllEvents(std::ostream &os) const;

private:
  // have we seen a call to __user_main() yet?
  // don't bother tracing anything until we see this,
  // or else we'll get junky prologue shit
  bool hasSeenUserMain;

  // ugh C++ only supports polymorphic calls thru pointers
  //
  // WARNING: these are NEVER FREED, because they are shared
  // across different states (when states fork), so we have 
  // an *intentional* memory leak, but oh wellz ;)
  std::vector<ExecutionTraceEvent*> events;
};


class ExecutionState {
public:
  typedef std::vector<StackFrame> stack_ty;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState&); 
  std::map< std::string, std::string > fnAliases;

public:
  bool fakeState;
  // Are we currently underconstrained?  Hack: value is size to make fake
  // objects.
  unsigned underConstrained;
  unsigned depth;
  
  // pc - pointer to current instruction stream
  KInstIterator pc, prevPC;
  stack_ty stack;
  ConstraintManager constraints;
  mutable double queryCost;
  double weight;
  AddressSpace addressSpace;
  TreeOStream pathOS, symPathOS;
  unsigned instsSinceCovNew;
  bool coveredNew;

  // for printing execution traces when this state terminates
  ExecutionTraceManager exeTraceMgr;

  /// Disables forking, set by user code.
  bool forkDisabled;

  std::map<const std::string*, std::set<unsigned> > coveredLines;
  PTreeNode *ptreeNode;

  /// ordered list of symbolics: used to generate test cases. 
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector< std::pair<const MemoryObject*, const Array*> > symbolics;

  // Used by the checkpoint/rollback methods for fake objects.
  // FIXME: not freeing things on branch deletion.
  MemoryMap shadowObjects;

  unsigned incomingBBIndex;

  std::string getFnAlias(std::string fn);
  void addFnAlias(std::string old_fn, std::string new_fn);
  void removeFnAlias(std::string fn);
  
private:
  ExecutionState() : fakeState(false), underConstrained(0), ptreeNode(0) {};

public:
  ExecutionState(KFunction *kf);

  // XXX total hack, just used to make a state so solver can
  // use on structure
  ExecutionState(const std::vector<ref<Expr> > &assumptions);

  ~ExecutionState();
  
  ExecutionState *branch();

  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame();

  void addSymbolic(const MemoryObject *mo, const Array *array) { 
    symbolics.push_back(std::make_pair(mo, array));
  }
  void addConstraint(ref<Expr> e) { 
    constraints.addConstraint(e); 
  }

  // Used for checkpoint/rollback of fake objects created during tainting.
  ObjectState *cloneObject(ObjectState *os, MemoryObject *mo);

  //

  bool merge(const ExecutionState &b);
};


// for producing abbreviated execution traces to help visualize
// paths and diagnose bugs

class ExecutionTraceEvent {
public:
  // the location of the instruction:
  std::string file;
  unsigned line;
  std::string funcName;
  unsigned stackDepth;

  unsigned consecutiveCount; // init to 1, increase for CONSECUTIVE
                             // repetitions of the SAME event

  ExecutionTraceEvent()
    : file("global"), line(0), funcName("global_def"),
      consecutiveCount(1) {}

  ExecutionTraceEvent(ExecutionState& state, KInstruction* ki);

  virtual ~ExecutionTraceEvent() {}

  void print(std::ostream &os) const;

  // return true if it shouldn't be added to ExecutionTraceManager
  //
  virtual bool ignoreMe() const;

private:
  virtual void printDetails(std::ostream &os) const = 0;
};


class FunctionCallTraceEvent : public ExecutionTraceEvent {
public:
  std::string calleeFuncName;

  FunctionCallTraceEvent(ExecutionState& state, KInstruction* ki,
                         const std::string& _calleeFuncName)
    : ExecutionTraceEvent(state, ki), calleeFuncName(_calleeFuncName) {}

private:
  virtual void printDetails(std::ostream &os) const {
    os << "CALL " << calleeFuncName;
  }

};

class FunctionReturnTraceEvent : public ExecutionTraceEvent {
public:
  FunctionReturnTraceEvent(ExecutionState& state, KInstruction* ki)
    : ExecutionTraceEvent(state, ki) {}

private:
  virtual void printDetails(std::ostream &os) const {
    os << "RETURN";
  }
};

class BranchTraceEvent : public ExecutionTraceEvent {
public:
  bool trueTaken;         // which side taken?
  bool canForkGoBothWays;

  BranchTraceEvent(ExecutionState& state, KInstruction* ki,
                   bool _trueTaken, bool _isTwoWay)
    : ExecutionTraceEvent(state, ki),
      trueTaken(_trueTaken),
      canForkGoBothWays(_isTwoWay) {}

private:
  virtual void printDetails(std::ostream &os) const;
};

}

#endif
