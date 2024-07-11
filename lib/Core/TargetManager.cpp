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

void TargetManager::pullGlobal(ExecutionState &state) {
  if (!isTargeted(state) || targets(state).size() == 0) {
    return;
  }

  auto &stateTargetForest = targetForest(state);

  for (auto &target : stateTargetForest.leafs()) {
    if (reachedTargets.count(target)) {
      stateTargetForest.block(target);
    }
  }
  setTargets(state, stateTargetForest.getTargets());
  if (guidance == Interpreter::GuidanceKind::CoverageGuidance) {
    if (targets(state).size() == 0) {
      state.setTargeted(false);
    }
  }
}

void TargetManager::updateMiss(ExecutionState &state, ref<Target> target) {
  auto &stateTargetForest = targetForest(state);
  stateTargetForest.remove(target);
  setTargets(state, stateTargetForest.getTargets());
  if (guidance == Interpreter::GuidanceKind::CoverageGuidance) {
    if (targets(state).size() == 0) {
      state.setTargeted(false);
    }
  }
}

void TargetManager::updateContinue(ExecutionState &, ref<Target>) {}

void TargetManager::updateDone(ExecutionState &state, ref<Target> target) {
  auto &stateTargetForest = targetForest(state);

  stateTargetForest.stepTo(target);
  setTargets(state, stateTargetForest.getTargets());
  setHistory(state, stateTargetForest.getHistory());
  if (guidance == Interpreter::GuidanceKind::CoverageGuidance ||
      target->shouldFailOnThisTarget()) {
    if (target->shouldFailOnThisTarget() ||
        !isa<KCallBlock>(target->getBlock())) {
      setReached(target);
    }
  }
  if (guidance == Interpreter::GuidanceKind::CoverageGuidance) {
    if (targets(state).size() == 0) {
      state.setTargeted(false);
    }
  }
}

void TargetManager::collect(ExecutionState &state) {
  if (!state.areTargetsChanged()) {
    assert(state.targets() == state.prevTargets());
    assert(state.history() == state.prevHistory());
    return;
  }

  ref<const TargetsHistory> prevHistory = state.prevHistory();
  ref<const TargetsHistory> history = state.history();
  const auto &prevTargets = state.prevTargets();
  const auto &targets = state.targets();
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
    addedTargets.insert(targets.begin(), targets.end());
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

void TargetManager::collectFiltered(
    const StatesSet &filter, TargetHistoryTargetPairToStatesMap &added,
    TargetHistoryTargetPairToStatesMap &removed) {
  for (auto htStatesPair : addedTStates) {
    for (auto &state : htStatesPair.second) {
      if (filter.count(state)) {
        added[htStatesPair.first].push_back(state);
        removed[htStatesPair.first];
      }
    }
  }
  for (auto htStatesPair : removedTStates) {
    for (auto &state : htStatesPair.second) {
      if (filter.count(state)) {
        removed[htStatesPair.first].push_back(state);
        added[htStatesPair.first];
      }
    }
  }
}

void TargetManager::updateReached(ExecutionState &state) {
  auto prevKI = state.prevPC;
  auto kf = prevKI->parent->parent;
  auto kmodule = kf->parent;

  if (prevKI->inst()->isTerminator() &&
      kmodule->inMainModule(*kf->function())) {
    ref<Target> target;

    if (state.getPrevPCBlock()->getTerminator()->getNumSuccessors() == 0) {
      target = ReachBlockTarget::create(state.prevPC->parent, true);
    } else if (isa<KCallBlock>(state.prevPC->parent) &&
               !targetCalculator.uncoveredBlockPredicate(
                   state.prevPC->parent)) {
      target = ReachBlockTarget::create(state.prevPC->parent, true);
    } else if (!isa<KCallBlock>(state.prevPC->parent)) {
      unsigned index = 0;
      for (auto succ : successors(state.getPrevPCBlock())) {
        if (succ == state.getPCBlock()) {
          target = CoverBranchTarget::create(state.prevPC->parent, index);
          break;
        }
        ++index;
      }
    }

    if (target && guidance == Interpreter::GuidanceKind::CoverageGuidance) {
      setReached(target);
      target = ReachBlockTarget::create(state.pc->parent, false);
      setReached(target);
    }
  }
}

void TargetManager::updateTargets(ExecutionState &state) {
  if (guidance == Interpreter::GuidanceKind::CoverageGuidance) {
    if (targets(state).empty() && state.isStuck(MaxCyclesBeforeStuck)) {
      state.setTargeted(true);
    }
    if (isTargeted(state) && targets(state).empty()) {
      TargetHashSet targets(targetCalculator.calculate(state));
      if (!targets.empty()) {
        state.targetForest.add(
            TargetForest::UnorderedTargetsSet::create(targets));
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

void TargetManager::update(ref<ObjectManager::Event> e) {
  switch (e->getKind()) {
  case ObjectManager::Event::Kind::States: {
    auto statesEvent = cast<ObjectManager::States>(e);
    update(statesEvent->modified, statesEvent->added, statesEvent->removed);
    break;
  }
  default:
    break;
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
    updateReached(*state);
    updateTargets(*state);
    if (state->areTargetsChanged()) {
      changedStates.insert(state);
    }
  }

  for (auto &pair : addedTStates) {
    pair.second.clear();
  }
  for (auto &pair : removedTStates) {
    pair.second.clear();
  }

  for (auto state : changedStates) {
    collect(*state);
    state->stepTargetsAndHistory();
  }

  for (const auto state : removedStates) {
    states.erase(state);
    distances.erase(state);
  }

  changedStates.clear();
  localStates.clear();
}

bool TargetManager::isReachedTarget(const ExecutionState &state,
                                    ref<Target> target) {
  WeightResult result;
  if (isReachedTarget(state, target, result)) {
    return result == Done;
  }
  return false;
}

bool TargetManager::isReachedTarget(const ExecutionState &state,
                                    ref<Target> target, WeightResult &result) {
  if (state.constraints.path().KBlockSize() == 0) {
    return false;
  }

  if (isa<ReachBlockTarget>(target)) {
    if (cast<ReachBlockTarget>(target)->isAtEnd()) {
      if (state.prevPC->parent == target->getBlock() ||
          state.pc->parent == target->getBlock()) {
        if (state.prevPC == target->getBlock()->getLastInstruction()) {
          result = Done;
        } else {
          result = Continue;
        }
        return true;
      }
    } else {
      if (state.pc->parent == target->getBlock()) {
        result = Done;
        return true;
      }
    }
  }

  if (isa<CoverBranchTarget>(target)) {
    if (state.prevPC->parent == target->getBlock()) {
      if (state.prevPC == target->getBlock()->getLastInstruction() &&
          state.prevPC->inst()->getSuccessor(
              cast<CoverBranchTarget>(target)->getBranchCase()) ==
              state.pc->parent->basicBlock()) {
        result = Done;
      } else {
        result = Continue;
      }
      return true;
    }
  }

  if (target->shouldFailOnThisTarget()) {
    bool found = true;
    auto possibleInstruction = state.prevPC;
    int i = state.stack.size() - 1;

    while (!cast<ReproduceErrorTarget>(target)->isTheSameAsIn(
        possibleInstruction)) { // TODO: target->getBlock() ==
                                // possibleInstruction should also be checked,
                                // but more smartly
      if (i <= 0) {
        found = false;
        break;
      }
      possibleInstruction = state.stack.callStack().at(i).caller;
      i--;
    }
    if (found) {
      if (cast<ReproduceErrorTarget>(target)->isThatError(state.error)) {
        result = Done;
      } else {
        result = Continue;
      }
      return true;
    }
  }
  return false;
}
