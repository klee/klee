//===-- TargetedExecutionManager.h ------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to manage everything for targeted execution mode
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TARGETEDEXECUTIONMANAGER_H
#define KLEE_TARGETEDEXECUTIONMANAGER_H

#include "klee/Core/TargetedExecutionReporter.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"
#include "klee/Module/TargetForest.h"

#include <unordered_map>

namespace klee {
extern llvm::cl::OptionCategory TerminationCat;

/*** Termination criteria options ***/

extern llvm::cl::opt<std::string> MaxTime;

extern llvm::cl::list<StateTerminationType> ExitOnErrorType;

extern llvm::cl::opt<unsigned long long> MaxInstructions;

extern llvm::cl::opt<unsigned long long> MaxSteppedInstructions;

extern llvm::cl::opt<unsigned> MaxForks;

extern llvm::cl::opt<unsigned> MaxDepth;

extern llvm::cl::opt<unsigned> MaxMemory;

extern llvm::cl::opt<bool> MaxMemoryInhibit;

extern llvm::cl::opt<unsigned> RuntimeMaxStackFrames;

extern llvm::cl::opt<double> MaxStaticForkPct;

extern llvm::cl::opt<double> MaxStaticSolvePct;

extern llvm::cl::opt<double> MaxStaticCPForkPct;

extern llvm::cl::opt<double> MaxStaticCPSolvePct;

extern llvm::cl::opt<unsigned> MaxStaticPctCheckDelay;

extern llvm::cl::opt<std::string> TimerInterval;

class CodeGraphDistance;

class LocatedEventManager {
  using FilenameCache = std::unordered_map<std::string, bool>;
  std::unordered_map<std::string, std::unique_ptr<FilenameCache>>
      filenameCacheMap;
  FilenameCache *filenameCache = nullptr;

public:
  LocatedEventManager() {}

  void prefetchFindFilename(const std::string &filename);

  bool isInside(Location &loc, const klee::FunctionInfo &fi);
};

class TargetedHaltsOnTraces {
  using HaltTypeToConfidence =
      std::unordered_map<HaltExecution::Reason, confidence::ty>;
  using TraceToHaltTypeToConfidence =
      std::unordered_map<ref<TargetForest::UnorderedTargetsSet>,
                         HaltTypeToConfidence,
                         TargetForest::RefUnorderedTargetsSetHash,
                         TargetForest::RefUnorderedTargetsSetCmp>;
  TraceToHaltTypeToConfidence traceToHaltTypeToConfidence;

  static void totalConfidenceAndTopContributor(
      const HaltTypeToConfidence &haltTypeToConfidence,
      confidence::ty *confidence, HaltExecution::Reason *reason);

public:
  explicit TargetedHaltsOnTraces(ref<TargetForest> &forest);

  void subtractConfidencesFrom(TargetForest &forest,
                               HaltExecution::Reason reason);

  /* Report for targeted static analysis mode */
  void reportFalsePositives(bool canReachSomeTarget);
};

class TargetedExecutionManager {
private:
  using Blocks = std::unordered_set<KBlock *>;
  using LocationToBlocks = std::unordered_map<ref<Location>, Blocks,
                                              RefLocationHash, RefLocationCmp>;
  using Locations =
      std::unordered_set<ref<Location>, RefLocationHash, RefLocationCmp>;

  using Instructions = std::unordered_map<
      std::string,
      std::unordered_map<
          unsigned int,
          std::unordered_map<unsigned int, std::unordered_set<unsigned int>>>>;
  std::unordered_set<unsigned> broken_traces;
  std::unordered_set<unsigned> reported_traces;

  bool tryResolveLocations(Result &result, LocationToBlocks &locToBlocks) const;
  LocationToBlocks prepareAllLocations(KModule *kmodule,
                                       Locations &locations) const;
  Locations collectAllLocations(const SarifReport &paths) const;

  bool canReach(const ref<Location> &from, const ref<Location> &to,
                LocationToBlocks &locToBlocks) const;

  KFunction *tryResolveEntryFunction(const Result &result,
                                     LocationToBlocks &locToBlocks) const;

  CodeGraphDistance &codeGraphDistance;

public:
  explicit TargetedExecutionManager(CodeGraphDistance &codeGraphDistance_)
      : codeGraphDistance(codeGraphDistance_) {}
  std::unordered_map<KFunction *, ref<TargetForest>>
  prepareTargets(KModule *kmodule, SarifReport paths);

  void reportFalseNegative(ExecutionState &state, ReachWithError error);

  // Return true if report is successful
  bool reportTruePositive(ExecutionState &state, ReachWithError error);
};

} // namespace klee

#endif /* KLEE_TARGETEDEXECUTIONMANAGER_H */
