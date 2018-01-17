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
#include "klee/MergeHandler.h"

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

  /*****************************************/
  /* OISH: this is pretty clear:           */
  /* save all the original C variabels     */
  /* these are the same variables that are */
  /* allocated with the alloca             */
  /*****************************************/  
  std::vector<const MemoryObject *> allocas;

  /*****************************************/
  /* OISH: this is *so* unclear:           */
  /* I have absolutely no idea what these  */
  /* locals are ...                        */
  /*****************************************/  
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

/// @brief ExecutionState representing a path under exploration
class ExecutionState {
public:
  typedef std::vector<StackFrame> stack_ty;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState &);

  std::map<std::string, std::string> fnAliases;

public:
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
  unsigned incomingBBIndex;

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
  /****************************************************************************/
  /* OISH: This is what I was looking for ... a connection between the actual */
  /* allocation (MemoryObject *) and the symbolic buffer (Klee::Array *)      */
  /* I need to be able to ask: "what is the (MemoryObject *) corresponding to */
  /* this (Klee::Array *) symbolic buffer ?                                   */
  /****************************************************************************/
  std::vector<std::pair<const MemoryObject *, const Array *> > symbolics;

  /// @brief Set of used array names for this state.  Used to avoid collisions.
  std::set<std::string> arrayNames;

  std::string getFnAlias(std::string fn);
  void addFnAlias(std::string old_fn, std::string new_fn);
  void removeFnAlias(std::string fn);

  // The objects handling the klee_open_merge calls this state ran through
  std::vector<ref<MergeHandler> > openMergeStack;

	int numABSerials=0;

	/****************************************/
	/* dummy integer just for debugging ... */
	/****************************************/
	int OrenIshShalom;

	/***********************/
	/* Local variables ... */
	/***********************/
	std::set<std::string> localVariables;

	/*****************************************************************************************/
	/* varNames is to enable easy name identification of variables                           */
	/* We basically allow ourselves to call local variables with the OISH_ prefix            */
	/* then, for every temporary called tmp, arrayidx7 etc. we simply locate it all          */
	/* the way up to the original local variable ... this bypasses a lot of klee's hardships */
	/*****************************************************************************************/
	std::map<std::string,std::string> varNames;

	/****************************/
	/* svars = string variables */
	/* ------------------------ */
	/* ------------------------ */
	/* ------------------------ */
	/* That is, their names ... */
	/****************************/
	std::set<std::string> svars;

	/***************************************************/
	/* serial : svars ------> N                        */
	/* serial = the way to identify abstract buffers   */
	/* ----------------------------------------------  */
	/* ----------------------------------------------  */
	/* ----------------------------------------------  */
	/* That is, each abstract buffer is represented by */
	/* a unique integer ...                            */
	/***************************************************/
	std::map<std::string,int> ab_serial;

	/*************************************************************/
	/* offset : svars ------> N                                  */
	/* offset = the offset of an svar inside its abstract buffer */
	/* --------------------------------------------------------  */
	/* --------------------------------------------------------  */
	/* --------------------------------------------------------  */
	/* This is over simplified for now -- the offset should      */
	/* later be a ref<Expr> ...                                  */
	/*************************************************************/
	std::map<std::string,ref<Expr> > ab_offset;

	/*********************************************************/
	/* size : AbstractBuffers ------> N+                     */
	/* ----------------------------------------------------- */
	/* ----------------------------------------------------- */
	/* ----------------------------------------------------- */
	/* That is, the size of each abstract buffer, referenced */
	/* by its serial number                                  */
	/*********************************************************/
	std::map<int,ref<Expr> > ab_size;

	/*********************************************************/
	/* last : AbstractBuffers ------> N                      */
	/* ----------------------------------------------------- */
	/* ----------------------------------------------------- */
	/* ----------------------------------------------------- */
	/* That is, the last version of an abstract buffer       */
	/*********************************************************/
	std::map<int,int> ab_last;

private:
  ExecutionState() : ptreeNode(0) {}

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
