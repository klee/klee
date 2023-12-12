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
    if (targets(state).size() == 0) {
      state.setTargeted(false);
    }
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
    if (target->shouldFailOnThisTarget() ||
        !isa<KCallBlock>(target->getBlock())) {
      reachedTargets.insert(target);
    }

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

void TargetManager::updateReached(ExecutionState &state) {
  auto prevKI = state.prevPC;
  auto kf = prevKI->parent->parent;
  auto kmodule = kf->parent;

  if (prevKI->inst()->isTerminator() &&
      kmodule->inMainModule(*kf->function())) {
    ref<Target> target;

    if (state.getPrevPCBlock()->getTerminator()->getNumSuccessors() == 0) {
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

  for (auto state : changedStates) {
    if (std::find(addedStates.begin(), addedStates.end(), state) ==
        addedStates.end()) {
      collect(*state);
    }
    state->stepTargetsAndHistory();
  }

  for (const auto state : removedStates) {
    states.erase(state);
    distances.erase(state);
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
