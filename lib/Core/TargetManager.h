//===-- TargetedManager.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to manage everything about targets
//
//===----------------------------------------------------------------------===//

#include "DistanceCalculator.h"

#include "klee/Core/Interpreter.h"
#include "klee/Module/TargetHash.h"

#include <map>
#include <unordered_map>

#ifndef KLEE_TARGETMANAGER_H
#define KLEE_TARGETMANAGER_H

namespace klee {
class TargetCalculator;

using TargetHistoryTargetPair =
    std::pair<ref<const TargetsHistory>, ref<Target>>;
using StatesVector = std::vector<ExecutionState *>;
using TargetHistoryTargetPairToStatesMap =
    std::unordered_map<TargetHistoryTargetPair, StatesVector,
                       TargetHistoryTargetHash, TargetHistoryTargetCmp>;

class TargetManagerSubscriber {
public:
  virtual ~TargetManagerSubscriber() = default;

  /// Selects a state for further exploration.
  /// \return The selected state.
  virtual void update(const TargetHistoryTargetPairToStatesMap &added,
                      const TargetHistoryTargetPairToStatesMap &removed) = 0;
};

class TargetManager {
private:
  using StatesSet = std::unordered_set<ExecutionState *>;
  using StateToDistanceMap =
      std::unordered_map<const ExecutionState *, TargetHashMap<DistanceResult>>;
  using TargetForestHistoryTargetSet =
      std::unordered_set<TargetHistoryTargetPair, TargetHistoryTargetHash,
                         TargetHistoryTargetCmp>;

  Interpreter::GuidanceKind guidance;
  DistanceCalculator &distanceCalculator;
  TargetCalculator &targetCalculator;
  TargetHashSet reachedTargets;
  StatesSet states;
  StateToDistanceMap distances;
  StatesSet localStates;
  StatesSet changedStates;
  std::vector<TargetManagerSubscriber *> subscribers;
  TargetHistoryTargetPairToStatesMap addedTStates;
  TargetHistoryTargetPairToStatesMap removedTStates;
  TargetHashSet removedTargets;
  TargetHashSet addedTargets;

  void setTargets(ExecutionState &state, const TargetHashSet &targets) {
    state.setTargets(targets);
    changedStates.insert(&state);
  }

  void setHistory(ExecutionState &state, ref<TargetsHistory> history) {
    state.setHistory(history);
    changedStates.insert(&state);
  }

  void updateMiss(ExecutionState &state, ref<Target> target);

  void updateContinue(ExecutionState &state, ref<Target> target);

  void updateDone(ExecutionState &state, ref<Target> target);

  void updateReached(ExecutionState &state);

  void updateTargets(ExecutionState &state);

  void collect(ExecutionState &state);

  bool isReachedTarget(const ExecutionState &state, ref<Target> target,
                       WeightResult &result);

public:
  TargetManager(Interpreter::GuidanceKind _guidance,
                DistanceCalculator &_distanceCalculator,
                TargetCalculator &_targetCalculator)
      : guidance(_guidance), distanceCalculator(_distanceCalculator),
        targetCalculator(_targetCalculator){};

  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates);

  DistanceResult distance(const ExecutionState &state, ref<Target> target) {

    WeightResult wresult;
    if (isReachedTarget(state, target, wresult)) {
      return DistanceResult(wresult);
    }

    if (!state.isTransfered() && distances[&state].count(target)) {
      return distances[&state][target];
    }

    DistanceResult result =
        distanceCalculator.getDistance(state, target->getBlock());

    if (Done == result.result && (!isa<ReachBlockTarget>(target) ||
                                  cast<ReachBlockTarget>(target)->isAtEnd())) {
      result.result = Continue;
    }

    distances[&state][target] = result;

    return result;
  }

  const PersistentSet<ref<Target>> &targets(const ExecutionState &state) {
    return state.targets();
  }

  ref<const TargetsHistory> history(const ExecutionState &state) {
    return state.history();
  }

  ref<const TargetsHistory> prevHistory(const ExecutionState &state) {
    return state.prevHistory();
  }

  const PersistentSet<ref<Target>> &prevTargets(const ExecutionState &state) {
    return state.prevTargets();
  }

  TargetForest &targetForest(ExecutionState &state) {
    return state.targetForest;
  }

  void subscribe(TargetManagerSubscriber &subscriber) {
    subscribers.push_back(&subscriber);
  }

  bool isTargeted(const ExecutionState &state) { return state.isTargeted(); }

  bool isReachedTarget(const ExecutionState &state, ref<Target> target);

  void setReached(ref<Target> target) { reachedTargets.insert(target); }
};

} // namespace klee

#endif /* KLEE_TARGETMANAGER_H */
