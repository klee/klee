//===-- StatsTracker.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_STATSTRACKER_H
#define KLEE_STATSTRACKER_H

#include "CallPathManager.h"

#include <set>

namespace llvm {
  class BranchInst;
  class Function;
  class Instruction;
  class raw_fd_ostream;
}

namespace klee {
  class ExecutionState;
  class Executor;  
  class InstructionInfoTable;
  class InterpreterHandler;
  struct KInstruction;
  struct StackFrame;

  class StatsTracker {
    friend class WriteStatsTimer;
    friend class WriteIStatsTimer;

    Executor &executor;
    std::string objectFilename;

    llvm::raw_fd_ostream *statsFile, *istatsFile;
    double startWallTime;
    
    unsigned numBranches;
    unsigned fullBranches, partialBranches;

    CallPathManager callPathManager;    

    bool updateMinDistToUncovered;

  public:
    static bool useStatistics();

  private:
    void updateStateStatistics(uint64_t addend);
    void writeStatsHeader();
    void writeStatsLine();
    void writeIStats();

  public:
    StatsTracker(Executor &_executor, std::string _objectFilename,
                 bool _updateMinDistToUncovered);
    ~StatsTracker();

    // called after a new StackFrame has been pushed (for callpath tracing)
    void framePushed(ExecutionState &es, StackFrame *parentFrame);

    // called after a StackFrame has been popped 
    void framePopped(ExecutionState &es);

    // called when some side of a branch has been visited. it is
    // imperative that this be called when the statistics index is at
    // the index for the branch itself.
    void markBranchVisited(ExecutionState *visitedTrue, 
                           ExecutionState *visitedFalse);
    
    // called when execution is done and stats files should be flushed
    void done();

    // process stats for a single instruction step, es is the state
    // about to be stepped
    void stepInstruction(ExecutionState &es);

    /// Return time in seconds since execution start.
    double elapsed();

    void computeReachableUncovered();
  };

  uint64_t computeMinDistToUncovered(const KInstruction *ki,
                                     uint64_t minDistAtRA);

}

#endif
