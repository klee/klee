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

  bool tryResolveLocations(Result &locations,
                           LocationToBlocks &locToBlocks) const;
  LocationToBlocks prepareAllLocations(KModule *kmodule,
                                       Locations &locations) const;
  Locations collectAllLocations(const SarifReport &paths) const;

public:
  std::unordered_map<KFunction *, ref<TargetForest>>
  prepareTargets(KModule *kmodule, SarifReport paths);

  void reportFalseNegative(ExecutionState &state, ReachWithError error);

  // Return true if report is successful
  bool reportTruePositive(ExecutionState &state, ReachWithError error);
};

} // namespace klee

#endif /* KLEE_TARGETEDEXECUTIONMANAGER_H */
