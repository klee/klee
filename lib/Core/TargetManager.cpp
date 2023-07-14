//===-- TargetedManager.cpp -----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TargetManager.h"

#include "TargetCalculator.h"

#include "klee/Module/KInstruction.h"

#include <cassert>

using namespace llvm;
using namespace klee;

namespace klee {} // namespace klee

void TargetManager::updateMiss(ExecutionState &state, ref<Target> target) {
  auto &stateTargetForest = targetForest(state);
  stateTargetForest.remove(target);
  setTargets(state, stateTargetForest.getTargets());
  if (guidance == Interpreter::GuidanceKind::CoverageGuidance) {
    state.setTargeted(false);
  }
}

void TargetManager::updateContinue(ExecutionState &state, ref<Target> target) {}

void TargetManager::updateDone(ExecutionState &state, ref<Target> target) {
  auto &stateTargetForest = targetForest(state);

  stateTargetForest.stepTo(target);
  setTargets(state, stateTargetForest.getTargets());
  setHistory(state, stateTargetForest.getHistory());
  if (guidance == Interpreter::GuidanceKind::CoverageGuidance ||
      target->shouldFailOnThisTarget()) {
    reachedTargets.insert(target);
    for (auto es : states) {
      if (isTargeted(*es)) {
        auto &esTargetForest = targetForest(*es);
        esTargetForest.block(target);
        setTargets(*es, esTargetForest.getTargets());
        if (guidance == Interpreter::GuidanceKind::CoverageGuidance) {
          if (targets(*es).size() == 0) {
            es->setTargeted(false);
          }
        }
      }
    }
  }
  if (guidance == Interpreter::GuidanceKind::CoverageGuidance) {
    state.setTargeted(false);
  }
}

void TargetManager::collect(ExecutionState &state) {
  if (!state.areTargetsChanged()) {
    return;
  }

  ref<const TargetsHistory> prevHistory = state.prevHistory();
  ref<const TargetsHistory> history = state.history();
  const TargetHashSet &prevTargets = state.prevTargets();
  const TargetHashSet &targets = state.targets();
  if (prevHistory != history) {
    for (auto target : prevTargets) {
      removedTStates[{prevHistory, target}].push_back(&state);
      addedTStates[{prevHistory, target}];
    }
    for (auto target : targets) {
      addedTStates[{history, target}].push_back(&state);
      removedTStates[{history, target}];
    }
  } else {
    addedTargets = targets;
    for (auto target : prevTargets) {
      if (addedTargets.erase(target) == 0) {
        removedTargets.insert(target);
      }
    }
    for (auto target : removedTargets) {
      removedTStates[{history, target}].push_back(&state);
      addedTStates[{history, target}];
    }
    for (auto target : addedTargets) {
      addedTStates[{history, target}].push_back(&state);
      removedTStates[{history, target}];
    }
    removedTargets.clear();
    addedTargets.clear();
  }
}

void TargetManager::updateTargets(ExecutionState &state) {
  if (guidance == Interpreter::GuidanceKind::CoverageGuidance) {
    if (targets(state).empty() && state.isStuck(MaxCyclesBeforeStuck)) {
      state.setTargeted(true);
    }
    if (isTargeted(state) && targets(state).empty()) {
      ref<Target> target(targetCalculator.calculate(state));
      if (target) {
        state.targetForest.add(target);
        setTargets(state, state.targetForest.getTargets());
      }
    }
  }

  if (!isTargeted(state)) {
    return;
  }

  auto stateTargets = targets(state);
  auto &stateTargetForest = targetForest(state);

  for (auto target : stateTargets) {
    if (!stateTargetForest.contains(target)) {
      continue;
    }

    DistanceResult stateDistance = distance(state, target);
    switch (stateDistance.result) {
    case WeightResult::Continue:
      updateContinue(state, target);
      break;
    case WeightResult::Miss:
      updateMiss(state, target);
      break;
    case WeightResult::Done:
      updateDone(state, target);
      break;
    default:
      assert(0 && "unreachable");
    }
  }
}

void TargetManager::update(ExecutionState *current,
                           const std::vector<ExecutionState *> &addedStates,
                           const std::vector<ExecutionState *> &removedStates) {

  states.insert(addedStates.begin(), addedStates.end());

  if (current && (std::find(removedStates.begin(), removedStates.end(),
                            current) == removedStates.end())) {
    localStates.insert(current);
  }
  for (const auto state : addedStates) {
    localStates.insert(state);
  }
  for (const auto state : removedStates) {
    localStates.insert(state);
  }

  for (auto state : localStates) {
    updateTargets(*state);
    if (state->areTargetsChanged()) {
      changedStates.insert(state);
    }
  }

  for (auto state : changedStates) {
    if (std::find(addedStates.begin(), addedStates.end(), state) ==
        addedStates.end()) {
      collect(*state);
    }
    state->stepTargetsAndHistory();
  }

  for (const auto state : removedStates) {
    states.erase(state);
  }

  for (auto subscriber : subscribers) {
    subscriber->update(addedTStates, removedTStates);
  }

  for (auto &pair : addedTStates) {
    pair.second.clear();
  }
  for (auto &pair : removedTStates) {
    pair.second.clear();
  }

  changedStates.clear();
  localStates.clear();
}
