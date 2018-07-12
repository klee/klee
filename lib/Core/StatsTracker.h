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

#include "llvm/Support/TimeValue.h"
#include <memory>
#include <set>

namespace llvm {
  class BranchInst;
  class Function;
  class Instruction;
  class raw_fd_ostream;
}

namespace klee {
class CoverageTracker;
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

  std::unique_ptr<llvm::raw_fd_ostream> statsFile, istatsFile;
  double startWallTime;

  llvm::sys::TimeValue lastNowTime;
  llvm::sys::TimeValue lastUserTime;

  /// Handles coverage-related tracking
  std::unique_ptr<CoverageTracker> coverageTracker;

  /// Indicates if statistics should be tracked
  bool instructionGranularityTracking;

  /// Handles tracking of call paths per state
  CallPathManager callPathManager;

  /// Indicates if tracking per call state is enabled
  bool codePathGranularityTracking;

private:
  /// Update state counter
  void updateStateStatistics(uint64_t addend);
  void writeStatsHeader();
  void writeStatsLine();
  void writeIStats();

public:
  ///
  /// @param executor Executor instance
  /// @param objectFilename for istats tracker
  StatsTracker(Executor &executor, std::string objectFilename);

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

  // Process and update statistics after the step has been made
  void postStepInstruction(ExecutionState &es);

  /// Return time in seconds since execution start.
  double elapsed();

  /// Requests tracking of instruction granularity
  void doTrackByInstruction();

  /// Requests tracking coverage of instruction and branch level
  void doTrackCoverage();

  void requestAutomaticDistanceMetricUpdates();

  /// Request tracking of code path granularity
  void doTrackCodePath();
};
}

#endif
