//===-- CoverageTracker.h ---------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LIB_CORE_COVERAGETRACKER_H_
#define LIB_CORE_COVERAGETRACKER_H_

#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

namespace klee {
class ExecutionState;
class Executor;
class KInstruction;
class KModule;
class StatsTracker;

class CoverageTracker {
  typedef std::map<const llvm::Instruction *,
                   std::vector<const llvm::Function *>>
      calltargets_ty;

  calltargets_ty callTargets;
  std::map<const llvm::Function *, std::vector<const llvm::Instruction *>>
      functionCallers;
  std::map<const llvm::Function *, unsigned> functionShortestPath;

  StatsTracker &statsTracker;
  KModule &kmodule;
  std::set<ExecutionState *> &states;

  bool initReachableUncovered;

  bool automaticDistanceUpdate;
  Executor &executor;

  void computeReachableUncovered(const KModule *km);

public:
  CoverageTracker(StatsTracker &tracker, KModule &km,
                  std::set<ExecutionState *> &_states, Executor &ex)
      : statsTracker(tracker), kmodule(km), states(_states),
        initReachableUncovered(false), automaticDistanceUpdate(false),
        executor(ex), numBranches(0), fullBranches(0), partialBranches(0){};

  void initializeCoverageTracking();

  /// Request different distance matrices be calculated automatically
  ///
  void requestAutomaticDistanceCalculation();

  void calculateDistanceToReturn();
  void computeReachableUncovered();

  void updateAllStateCoverage();

  void stepInstruction(ExecutionState &es);

  /// Check if the user explicitly requests coverage tracking
  /// @return true, if requested, otherwise false
  static bool userRequestedCoverageTracking();

  // For branch-based tracking
  void markBranchVisited(ExecutionState *visitedTrue,
                         ExecutionState *visitedFalse);
  // Tracking on branch level
  unsigned numBranches;
  unsigned fullBranches, partialBranches;

  void updateFrameCoverage(ExecutionState &es);
};

/// Computes the distance to the next uncovered instruction
///
/// @param ki the instruction to start with
/// @param minDistAtRA minimum distance to uncovered instruction on return (i.e.
/// the actual parent stackframe)
/// @return distance calculated
uint64_t computeMinDistToUncovered(const KInstruction *ki,
                                   uint64_t minDistAtRA);

} // namespace klee

#endif /* LIB_CORE_COVERAGETRACKER_H_ */
