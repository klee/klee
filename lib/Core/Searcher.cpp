//===-- Searcher.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Searcher.h"

#include "CoreStats.h"
#include "ExecutionState.h"
#include "Executor.h"
#include "PTree.h"
#include "StatsTracker.h"
#include "TargetCalculator.h"

#include "klee/ADT/DiscretePDF.h"
#include "klee/ADT/RNG.h"
#include "klee/ADT/WeightedQueue.h"
#include "klee/Module/CodeGraphDistance.h"
#include "klee/Module/InstructionInfoTable.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"
#include "klee/Statistics/Statistics.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/System/Time.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"

#include <cassert>
#include <cmath>
#include <set>

using namespace klee;
using namespace llvm;

///

ExecutionState &DFSSearcher::selectState() { return *states.back(); }

void DFSSearcher::update(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates) {
  // insert states
  states.insert(states.end(), addedStates.begin(), addedStates.end());

  // remove states
  for (const auto state : removedStates) {
    if (state == states.back()) {
      states.pop_back();
    } else {
      auto it = std::find(states.begin(), states.end(), state);
      assert(it != states.end() && "invalid state removed");
      states.erase(it);
    }
  }
}

bool DFSSearcher::empty() { return states.empty(); }

void DFSSearcher::printName(llvm::raw_ostream &os) { os << "DFSSearcher\n"; }

///

ExecutionState &BFSSearcher::selectState() { return *states.front(); }

void BFSSearcher::update(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates) {
  // update current state
  // Assumption: If new states were added KLEE forked, therefore states evolved.
  // constraints were added to the current state, it evolved.
  if (!addedStates.empty() && current &&
      std::find(removedStates.begin(), removedStates.end(), current) ==
          removedStates.end()) {
    auto pos = std::find(states.begin(), states.end(), current);
    assert(pos != states.end());
    states.erase(pos);
    states.push_back(current);
  }

  // insert states
  states.insert(states.end(), addedStates.begin(), addedStates.end());

  // remove states
  for (const auto state : removedStates) {
    if (state == states.front()) {
      states.pop_front();
    } else {
      auto it = std::find(states.begin(), states.end(), state);
      assert(it != states.end() && "invalid state removed");
      states.erase(it);
    }
  }
}

bool BFSSearcher::empty() { return states.empty(); }

void BFSSearcher::printName(llvm::raw_ostream &os) { os << "BFSSearcher\n"; }

///

RandomSearcher::RandomSearcher(RNG &rng) : theRNG{rng} {}

ExecutionState &RandomSearcher::selectState() {
  return *states[theRNG.getInt32() % states.size()];
}

void RandomSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  // insert states
  states.insert(states.end(), addedStates.begin(), addedStates.end());

  // remove states
  for (const auto state : removedStates) {
    auto it = std::find(states.begin(), states.end(), state);
    assert(it != states.end() && "invalid state removed");
    states.erase(it);
  }
}

bool RandomSearcher::empty() { return states.empty(); }

void RandomSearcher::printName(llvm::raw_ostream &os) {
  os << "RandomSearcher\n";
}

///

static unsigned int ulog2(unsigned int val) {
  if (val == 0)
    return UINT_MAX;
  if (val == 1)
    return 0;
  unsigned int ret = 0;
  while (val > 1) {
    val >>= 1;
    ret++;
  }
  return ret;
}

///

TargetedSearcher::TargetedSearcher(ref<Target> target,
                                   CodeGraphDistance &_distance)
    : states(std::make_unique<
             WeightedQueue<ExecutionState *, ExecutionStateIDCompare>>()),
      target(target), codeGraphDistance(_distance),
      distanceToTargetFunction(
          codeGraphDistance.getBackwardDistance(target->getBlock()->parent)) {}

ExecutionState &TargetedSearcher::selectState() { return *states->choose(0); }

bool TargetedSearcher::distanceInCallGraph(KFunction *kf, KBlock *kb,
                                           unsigned int &distance) {
  distance = UINT_MAX;
  const std::unordered_map<KBlock *, unsigned> &dist =
      codeGraphDistance.getDistance(kb);
  KBlock *targetBB = target->getBlock();
  KFunction *targetF = targetBB->parent;

  if (kf == targetF && dist.count(targetBB) != 0) {
    distance = 0;
    return true;
  }

  for (auto &kCallBlock : kf->kCallBlocks) {
    if (dist.count(kCallBlock) != 0) {
      for (auto &calledFunction : kCallBlock->calledFunctions) {
        KFunction *calledKFunction = kf->parent->functionMap[calledFunction];
        if (distanceToTargetFunction.count(calledKFunction) != 0 &&
            distance > distanceToTargetFunction.at(calledKFunction) + 1) {
          distance = distanceToTargetFunction.at(calledKFunction) + 1;
        }
      }
    }
  }
  return distance != UINT_MAX;
}

TargetedSearcher::WeightResult
TargetedSearcher::tryGetLocalWeight(ExecutionState *es, weight_type &weight,
                                    const std::vector<KBlock *> &localTargets) {
  unsigned int intWeight = es->steppedMemoryInstructions;
  KFunction *currentKF = es->pc->parent->parent;
  KBlock *initKB = es->initPC->parent;
  KBlock *currentKB = currentKF->blockMap[es->getPCBlock()];
  KBlock *prevKB = currentKF->blockMap[es->getPrevPCBlock()];
  const std::unordered_map<KBlock *, unsigned> &dist =
      codeGraphDistance.getDistance(currentKB);
  unsigned int localWeight = UINT_MAX;
  for (auto &end : localTargets) {
    if (dist.count(end) > 0) {
      unsigned int w = dist.at(end);
      localWeight = std::min(w, localWeight);
    }
  }

  if (localWeight == UINT_MAX)
    return Miss;
  if (localWeight == 0 && (initKB == currentKB || prevKB != currentKB ||
                           target->shouldFailOnThisTarget())) {
    return Done;
  }

  intWeight += localWeight;
  weight = ulog2(intWeight); // number on [0,32)-discrete-interval
  return Continue;
}

TargetedSearcher::WeightResult
TargetedSearcher::tryGetPreTargetWeight(ExecutionState *es,
                                        weight_type &weight) {
  KFunction *currentKF = es->pc->parent->parent;
  std::vector<KBlock *> localTargets;
  for (auto &kCallBlock : currentKF->kCallBlocks) {
    for (auto &calledFunction : kCallBlock->calledFunctions) {
      KFunction *calledKFunction =
          currentKF->parent->functionMap[calledFunction];
      if (distanceToTargetFunction.count(calledKFunction) > 0) {
        localTargets.push_back(kCallBlock);
      }
    }
  }

  if (localTargets.empty())
    return Miss;

  WeightResult res = tryGetLocalWeight(es, weight, localTargets);
  weight = weight + 32; // number on [32,64)-discrete-interval
  return res == Done ? Continue : res;
}

TargetedSearcher::WeightResult
TargetedSearcher::tryGetPostTargetWeight(ExecutionState *es,
                                         weight_type &weight) {
  KFunction *currentKF = es->pc->parent->parent;
  std::vector<KBlock *> &localTargets = currentKF->returnKBlocks;

  if (localTargets.empty())
    return Miss;

  WeightResult res = tryGetLocalWeight(es, weight, localTargets);
  weight = weight + 32; // number on [32,64)-discrete-interval
  return res == Done ? Continue : res;
}

TargetedSearcher::WeightResult
TargetedSearcher::tryGetTargetWeight(ExecutionState *es, weight_type &weight) {
  std::vector<KBlock *> localTargets = {target->getBlock()};
  WeightResult res = tryGetLocalWeight(es, weight, localTargets);
  return res;
}

TargetedSearcher::WeightResult
TargetedSearcher::tryGetWeight(ExecutionState *es, weight_type &weight) {
  if (target->atReturn() && !target->shouldFailOnThisTarget()) {
    if (es->prevPC->parent == target->getBlock() &&
        es->prevPC == target->getBlock()->getLastInstruction()) {
      return Done;
    } else if (es->pc->parent == target->getBlock()) {
      weight = 0;
      return Continue;
    }
  }

  if (target->shouldFailOnThisTarget() && target->isTheSameAsIn(es->prevPC) &&
      target->isThatError(es->error)) {
    return Done;
  }

  BasicBlock *bb = es->getPCBlock();
  KBlock *kb = es->pc->parent->parent->blockMap[bb];
  KInstruction *ki = es->pc;
  if (!target->shouldFailOnThisTarget() && kb->numInstructions &&
      !isa<KCallBlock>(kb) && kb->getFirstInstruction() != ki &&
      states->tryGetWeight(es, weight)) {
    return Continue;
  }
  unsigned int minCallWeight = UINT_MAX, minSfNum = UINT_MAX, sfNum = 0;
  for (auto sfi = es->stack.rbegin(), sfe = es->stack.rend(); sfi != sfe;
       sfi++) {
    unsigned callWeight;
    if (distanceInCallGraph(sfi->kf, kb, callWeight)) {
      callWeight *= 2;
      if (callWeight == 0 && target->shouldFailOnThisTarget()) {
        weight = 0;
        return target->isTheSameAsIn(kb->getFirstInstruction()) &&
                       target->isThatError(es->error)
                   ? Done
                   : Continue;
      } else {
        callWeight += sfNum;
      }

      if (callWeight < minCallWeight) {
        minCallWeight = callWeight;
        minSfNum = sfNum;
      }
    }

    if (sfi->caller) {
      kb = sfi->caller->parent;
      bb = kb->basicBlock;
    }
    sfNum++;

    if (minCallWeight < sfNum)
      break;
  }

  WeightResult res = Miss;
  if (minCallWeight == 0)
    res = tryGetTargetWeight(es, weight);
  else if (minSfNum == 0)
    res = tryGetPreTargetWeight(es, weight);
  else if (minSfNum != UINT_MAX)
    res = tryGetPostTargetWeight(es, weight);
  if (Done == res && target->shouldFailOnThisTarget()) {
    if (!target->isThatError(es->error)) {
      res = Continue;
    }
  }
  return res;
}

void TargetedSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  updateCheckCanReach(current, addedStates, removedStates);
}

bool TargetedSearcher::updateCheckCanReach(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  weight_type weight = 0;
  bool canReach = false;

  // update current
  if (current && std::find(removedStates.begin(), removedStates.end(),
                           current) == removedStates.end()) {
    switch (tryGetWeight(current, weight)) {
    case Continue:
      states->update(current, weight);
      canReach = true;
      break;
    case Done:
      reachedOnLastUpdate.insert(current);
      canReach = true;
      break;
    case Miss:
      current->targetForest.remove(target);
      states->remove(current);
      break;
    }
  }

  // insert states
  for (const auto state : addedStates) {
    switch (tryGetWeight(state, weight)) {
    case Continue:
      states->insert(state, weight);
      canReach = true;
      break;
    case Done:
      states->insert(state, weight);
      reachedOnLastUpdate.insert(state);
      canReach = true;
      break;
    case Miss:
      state->targetForest.remove(target);
      break;
    }
  }

  // remove states
  for (const auto state : removedStates) {
    if (target->atReturn() && !target->shouldFailOnThisTarget() &&
        state->prevPC == target->getBlock()->getLastInstruction()) {
      canReach = true;
      reachedOnLastUpdate.insert(state);
    } else {
      switch (tryGetWeight(state, weight)) {
      case Done:
        reachedOnLastUpdate.insert(state);
        canReach = true;
        break;
      case Miss:
        state->targetForest.remove(target);
        states->remove(state);
        break;
      case Continue:
        states->remove(state);
        canReach = true;
        break;
      }
    }
  }
  return canReach;
}

bool TargetedSearcher::empty() { return states->empty(); }

void TargetedSearcher::printName(llvm::raw_ostream &os) {
  os << "TargetedSearcher";
}

TargetedSearcher::~TargetedSearcher() {
  while (!states->empty()) {
    auto &state = selectState();
    state.targetForest.remove(target);
    states->remove(&state);
  }
}

std::set<ExecutionState *> TargetedSearcher::reached() {
  return reachedOnLastUpdate;
}

void TargetedSearcher::removeReached() {
  for (auto state : reachedOnLastUpdate) {
    states->remove(state);
    state->targetForest.stepTo(target);
  }
  reachedOnLastUpdate.clear();
}

///

GuidedSearcher::GuidedSearcher(
    CodeGraphDistance &codeGraphDistance, TargetCalculator &stateHistory,
    std::set<ExecutionState *, ExecutionStateIDCompare> &pausedStates,
    unsigned long long bound, RNG &rng, Searcher *baseSearcher)
    : baseSearcher(baseSearcher), codeGraphDistance(codeGraphDistance),
      stateHistory(stateHistory), pausedStates(pausedStates), bound(bound),
      theRNG(rng) {
  guidance = baseSearcher ? CoverageGuidance : ErrorGuidance;
}

ExecutionState &GuidedSearcher::selectState() {
  unsigned size = historiesAndTargets.size();
  index = theRNG.getInt32() % (size + 1);
  ExecutionState *state = nullptr;
  if (CoverageGuidance == guidance && index == size) {
    state = &baseSearcher->selectState();
  } else {
    index = index % size;
    auto &historyTargetPair = historiesAndTargets[index];
    ref<TargetForest::History> history = historyTargetPair.first;
    ref<Target> target = historyTargetPair.second;
    assert(targetedSearchers.find(history) != targetedSearchers.end() &&
           targetedSearchers.at(history).find(target) !=
               targetedSearchers.at(history).end() &&
           !targetedSearchers.at(history)[target]->empty());
    state = &targetedSearchers.at(history).at(target)->selectState();
  }
  return *state;
}

bool GuidedSearcher::isStuck(ExecutionState &state) {
  KInstruction *prevKI = state.prevPC;
  return (prevKI->inst->isTerminator() &&
          state.multilevel[state.getPCBlock()] > bound);
}

bool GuidedSearcher::updateTargetedSearcher(
    ref<TargetForest::History> history, ref<Target> target,
    ExecutionState *current, std::vector<ExecutionState *> &addedStates,
    std::vector<ExecutionState *> &removedStates) {
  bool canReach = false;
  auto &historiedTargetedSearchers = targetedSearchers[history];

  if (historiedTargetedSearchers.count(target) != 0 ||
      tryAddTarget(history, target)) {

    if (current) {
      assert(current->targetForest.contains(target));
      for (auto &reached : reachedTargets) {
        current->targetForest.blockIn(target, reached);
      }
      if (!current->targetForest.contains(target)) {
        removedStates.push_back(current);
      }
    }
    for (std::vector<ExecutionState *>::iterator is = addedStates.begin(),
                                                 ie = addedStates.end();
         is != ie;) {
      ExecutionState *state = *is;
      assert(state->targetForest.contains(target));
      for (auto &reached : reachedTargets) {
        state->targetForest.blockIn(target, reached);
      }
      if (!state->targetForest.contains(target)) {
        is = addedStates.erase(is);
        ie = addedStates.end();
      } else {
        ++is;
      }
    }

    canReach = historiedTargetedSearchers[target]->updateCheckCanReach(
        current, addedStates, removedStates);
    if (historiedTargetedSearchers[target]->empty()) {
      removeTarget(history, target);
    }
  } else if (isReached(history, target)) {
    canReach = true;
    if (current) {
      current->targetForest.remove(target);
    }
    assert(removedStates.empty());
    for (auto &state : addedStates) {
      state->targetForest.remove(target);
    }
  }
  if (targetedSearchers[history].empty())
    targetedSearchers.erase(history);
  return canReach;
}

static void updateConfidences(ExecutionState *current,
                              const GuidedSearcher::TargetToStateUnorderedSetMap
                                  &reachableStatesOfTarget) {
  if (current)
    current->targetForest.divideConfidenceBy(reachableStatesOfTarget);
}

void GuidedSearcher::updateTargetedSearcherForStates(
    std::vector<ExecutionState *> &states,
    std::vector<ExecutionState *> &tmpAddedStates,
    std::vector<ExecutionState *> &tmpRemovedStates,
    TargetToStateUnorderedSetMap &reachableStatesOfTarget, bool areStatesStuck,
    bool areStatesRemoved) {
  for (const auto state : states) {
    auto history = state->targetForest.getHistory();
    auto targets = state->targetForest.getTargets();
    TargetForest::TargetsSet stateTargets;
    for (auto &targetF : *targets) {
      stateTargets.insert(targetF.first);
    }

    for (auto &target : stateTargets) {
      if (areStatesRemoved || state->targetForest.contains(target)) {
        if (areStatesRemoved)
          tmpRemovedStates.push_back(state);
        else
          tmpAddedStates.push_back(state);
        assert(!state->targetForest.empty());

        bool canReach = areStatesStuck; // overapproximation: assume that stuck
                                        // state can reach any target
        if (!areStatesStuck)
          canReach = updateTargetedSearcher(history, target, nullptr,
                                            tmpAddedStates, tmpRemovedStates);
        if (canReach)
          reachableStatesOfTarget[target].insert(state);
      }
      tmpAddedStates.clear();
      tmpRemovedStates.clear();
    }
  }
}

void GuidedSearcher::innerUpdate(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  baseAddedStates.insert(baseAddedStates.end(), addedStates.begin(),
                         addedStates.end());
  baseRemovedStates.insert(baseRemovedStates.end(), removedStates.begin(),
                           removedStates.end());

  std::vector<ExecutionState *> addedStuckStates;
  if (ErrorGuidance == guidance) {
    if (current &&
        (std::find(baseRemovedStates.begin(), baseRemovedStates.end(),
                   current) == baseRemovedStates.end()) &&
        isStuck(*current)) {
      pausedStates.insert(current);
      baseRemovedStates.push_back(current);
    }
    for (const auto state : addedStates) {
      if (isStuck(*state)) {
        pausedStates.insert(state);
        addedStuckStates.push_back(state);
        auto is =
            std::find(baseAddedStates.begin(), baseAddedStates.end(), state);
        assert(is != baseAddedStates.end());
        baseAddedStates.erase(is);
      }
    }
  }

  for (const auto state : baseAddedStates) {
    if (!state->targetForest.empty()) {
      targetedAddedStates.push_back(state);
    } else {
      targetlessStates.push_back(state);
    }
  }

  TargetForest::TargetsSet currTargets;
  if (current) {
    auto targets = current->targetForest.getTargets();
    for (auto &targetF : *targets) {
      auto target = targetF.first;
      assert(target && "Target should be not null!");
      currTargets.insert(target);
    }
  }

  if (current && currTargets.empty() &&
      std::find(baseRemovedStates.begin(), baseRemovedStates.end(), current) ==
          baseRemovedStates.end()) {
    targetlessStates.push_back(current);
  }

  if (ErrorGuidance == guidance) {
    if (!removedStates.empty()) {
      std::vector<ExecutionState *> alt = removedStates;
      for (const auto state : alt) {
        auto it = pausedStates.find(state);
        if (it != pausedStates.end()) {
          pausedStates.erase(it);
          baseRemovedStates.erase(std::remove(baseRemovedStates.begin(),
                                              baseRemovedStates.end(), state),
                                  baseRemovedStates.end());
        }
      }
    }
  }

  std::vector<ExecutionState *> tmpAddedStates;
  std::vector<ExecutionState *> tmpRemovedStates;

  if (CoverageGuidance == guidance) {
    for (const auto state : targetlessStates) {
      if (isStuck(*state)) {
        ref<Target> target(stateHistory.calculate(*state));
        if (target) {
          state->targetForest.add(target);
          auto history = state->targetForest.getHistory();
          tmpAddedStates.push_back(state);
          updateTargetedSearcher(history, target, nullptr, tmpAddedStates,
                                 tmpRemovedStates);
          auto is = std::find(targetedAddedStates.begin(),
                              targetedAddedStates.end(), state);
          if (is != targetedAddedStates.end()) {
            targetedAddedStates.erase(is);
          }
          tmpAddedStates.clear();
          tmpRemovedStates.clear();
        } else {
          pausedStates.insert(state);
          if (std::find(baseAddedStates.begin(), baseAddedStates.end(),
                        state) != baseAddedStates.end()) {
            auto is = std::find(baseAddedStates.begin(), baseAddedStates.end(),
                                state);
            baseAddedStates.erase(is);
          } else {
            baseRemovedStates.push_back(state);
          }
        }
      }
    }
  }
  targetlessStates.clear();

  TargetToStateUnorderedSetMap reachableStatesOfTarget;

  if (current && !currTargets.empty()) {
    auto history = current->targetForest.getHistory();
    for (auto &target : currTargets) {
      bool canReach = false;
      if (current->targetForest.contains(target)) {
        canReach = updateTargetedSearcher(history, target, current,
                                          tmpAddedStates, tmpRemovedStates);
      } else {
        tmpRemovedStates.push_back(current);
        canReach = updateTargetedSearcher(history, target, nullptr,
                                          tmpAddedStates, tmpRemovedStates);
      }
      if (canReach)
        reachableStatesOfTarget[target].insert(current);
      tmpAddedStates.clear();
      tmpRemovedStates.clear();
    }
  }

  updateTargetedSearcherForStates(targetedAddedStates, tmpAddedStates,
                                  tmpRemovedStates, reachableStatesOfTarget,
                                  false, false);
  targetedAddedStates.clear();
  updateTargetedSearcherForStates(addedStuckStates, tmpAddedStates,
                                  tmpRemovedStates, reachableStatesOfTarget,
                                  true, false);
  updateTargetedSearcherForStates(baseRemovedStates, tmpAddedStates,
                                  tmpRemovedStates, reachableStatesOfTarget,
                                  false, true);

  if (CoverageGuidance == guidance) {
    assert(baseSearcher);
    baseSearcher->update(current, baseAddedStates, baseRemovedStates);
  }

  if (ErrorGuidance == guidance) {
    updateConfidences(current, reachableStatesOfTarget);
    for (auto state : baseAddedStates)
      updateConfidences(state, reachableStatesOfTarget);
    for (auto state : addedStuckStates)
      updateConfidences(state, reachableStatesOfTarget);
  }
  baseAddedStates.clear();
  baseRemovedStates.clear();
}

void GuidedSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  innerUpdate(current, addedStates, removedStates);
  clearReached(removedStates);
}

void GuidedSearcher::collectReached(TargetToStateSetMap &reachedStates) {
  for (auto &historyTarget : historiesAndTargets) {
    auto &history = historyTarget.first;
    auto &target = historyTarget.second;
    auto reached = targetedSearchers.at(history).at(target)->reached();
    if (!reached.empty()) {
      for (auto state : reached)
        reachedStates[target].insert(state);
    }
  }
}

void GuidedSearcher::clearReached(
    const std::vector<ExecutionState *> &removedStates) {
  std::vector<ExecutionState *> addedStates;
  std::vector<ExecutionState *> tmpAddedStates;
  std::vector<ExecutionState *> tmpRemovedStates;
  TargetToStateSetMap reachedStates;
  collectReached(reachedStates);

  for (auto &targetState : reachedStates) {
    auto target = targetState.first;
    auto states = targetState.second;
    for (auto &state : states) {
      auto history = state->targetForest.getHistory();
      auto targets = state->targetForest.getTargets();
      if (std::find(removedStates.begin(), removedStates.end(), state) ==
          removedStates.end()) {
        TargetForest::TargetsSet stateTargets;
        for (auto &targetF : *targets) {
          stateTargets.insert(targetF.first);
        }
        tmpRemovedStates.push_back(state);
        for (auto &anotherTarget : stateTargets) {
          if (target != anotherTarget) {
            updateTargetedSearcher(history, anotherTarget, nullptr,
                                   tmpAddedStates, tmpRemovedStates);
          }
        }
        tmpRemovedStates.clear();
        addedStates.push_back(state);
      }
    }
  }

  if (!reachedStates.empty()) {
    for (auto &targetState : reachedStates) {
      auto target = targetState.first;
      auto states = targetState.second;
      for (auto &state : states) {
        auto history = state->targetForest.getHistory();
        if (target->shouldFailOnThisTarget()) {
          reachedTargets.insert(target);
        }
        targetedSearchers.at(history).at(target)->removeReached();
        if (CoverageGuidance == guidance ||
            (ErrorGuidance == guidance && target->shouldFailOnThisTarget()) ||
            targetedSearchers.at(history).at(target)->empty()) {
          removeTarget(history, target);
        }
        if (targetedSearchers.at(history).empty())
          targetedSearchers.erase(history);
      }
    }
  }

  for (const auto state : addedStates) {
    auto history = state->targetForest.getHistory();
    auto targets = state->targetForest.getTargets();
    TargetForest::TargetsSet stateTargets;
    for (auto &targetF : *targets) {
      stateTargets.insert(targetF.first);
    }
    for (auto &target : stateTargets) {
      if (state->targetForest.contains(target)) {
        tmpAddedStates.push_back(state);
        updateTargetedSearcher(history, target, nullptr, tmpAddedStates,
                               tmpRemovedStates);
      }
      tmpAddedStates.clear();
      tmpRemovedStates.clear();
    }
  }
}

bool GuidedSearcher::empty() {
  return CoverageGuidance == guidance ? baseSearcher->empty()
                                      : targetedSearchers.empty();
}

void GuidedSearcher::printName(llvm::raw_ostream &os) {
  os << "GuidedSearcher\n";
}

bool GuidedSearcher::isReached(ref<TargetForest::History> history,
                               ref<Target> target) {
  return reachedTargets.count(target) != 0;
}

bool GuidedSearcher::tryAddTarget(ref<TargetForest::History> history,
                                  ref<Target> target) {
  if (isReached(history, target)) {
    return false;
  }
  assert(targetedSearchers.count(history) == 0 ||
         targetedSearchers.at(history).count(target) == 0);
  targetedSearchers[history][target] =
      std::make_unique<TargetedSearcher>(target, codeGraphDistance);
  auto it = std::find_if(
      historiesAndTargets.begin(), historiesAndTargets.end(),
      [&history, &target](
          const std::pair<ref<TargetForest::History>, ref<Target>> &element) {
        return element.first.get() == history.get() &&
               element.second.get() == target.get();
      });
  assert(it == historiesAndTargets.end());
  historiesAndTargets.push_back({history, target});
  return true;
}

GuidedSearcher::TargetForestHisoryTargetVector::iterator
GuidedSearcher::removeTarget(ref<TargetForest::History> history,
                             ref<Target> target) {
  targetedSearchers.at(history).erase(target);
  auto it = std::find_if(
      historiesAndTargets.begin(), historiesAndTargets.end(),
      [&history, &target](
          const std::pair<ref<TargetForest::History>, ref<Target>> &element) {
        return element.first.get() == history.get() &&
               element.second.get() == target.get();
      });
  assert(it != historiesAndTargets.end());
  return historiesAndTargets.erase(it);
}

///

WeightedRandomSearcher::WeightedRandomSearcher(WeightType type, RNG &rng)
    : states(std::make_unique<
             DiscretePDF<ExecutionState *, ExecutionStateIDCompare>>()),
      theRNG{rng}, type(type) {

  switch (type) {
  case Depth:
  case RP:
    updateWeights = false;
    break;
  case InstCount:
  case CPInstCount:
  case QueryCost:
  case MinDistToUncovered:
  case CoveringNew:
    updateWeights = true;
    break;
  default:
    assert(0 && "invalid weight type");
  }
}

ExecutionState &WeightedRandomSearcher::selectState() {
  return *states->choose(theRNG.getDoubleL());
}

double WeightedRandomSearcher::getWeight(ExecutionState *es) {
  switch (type) {
  default:
  case Depth:
    return es->depth;
  case RP:
    return std::pow(0.5, es->depth);
  case InstCount: {
    uint64_t count = theStatisticManager->getIndexedValue(stats::instructions,
                                                          es->pc->info->id);
    double inv = 1. / std::max((uint64_t)1, count);
    return inv * inv;
  }
  case CPInstCount: {
    StackFrame &sf = es->stack.back();
    uint64_t count = sf.callPathNode->statistics.getValue(stats::instructions);
    double inv = 1. / std::max((uint64_t)1, count);
    return inv;
  }
  case QueryCost:
    return (es->queryMetaData.queryCost.toSeconds() < .1)
               ? 1.
               : 1. / es->queryMetaData.queryCost.toSeconds();
  case CoveringNew:
  case MinDistToUncovered: {
    uint64_t md2u = computeMinDistToUncovered(
        es->pc, es->stack.back().minDistToUncoveredOnReturn);

    double invMD2U = 1. / (md2u ? md2u : 10000);
    if (type == CoveringNew) {
      double invCovNew = 0.;
      if (es->instsSinceCovNew)
        invCovNew = 1. / std::max(1, (int)es->instsSinceCovNew - 1000);
      return (invCovNew * invCovNew + invMD2U * invMD2U);
    } else {
      return invMD2U * invMD2U;
    }
  }
  }
}

void WeightedRandomSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {

  // update current
  if (current && updateWeights &&
      std::find(removedStates.begin(), removedStates.end(), current) ==
          removedStates.end())
    states->update(current, getWeight(current));

  // insert states
  for (const auto state : addedStates)
    states->insert(state, getWeight(state));

  // remove states
  for (const auto state : removedStates)
    states->remove(state);
}

bool WeightedRandomSearcher::empty() { return states->empty(); }

void WeightedRandomSearcher::printName(llvm::raw_ostream &os) {
  os << "WeightedRandomSearcher::";
  switch (type) {
  case Depth:
    os << "Depth\n";
    return;
  case RP:
    os << "RandomPath\n";
    return;
  case QueryCost:
    os << "QueryCost\n";
    return;
  case InstCount:
    os << "InstCount\n";
    return;
  case CPInstCount:
    os << "CPInstCount\n";
    return;
  case MinDistToUncovered:
    os << "MinDistToUncovered\n";
    return;
  case CoveringNew:
    os << "CoveringNew\n";
    return;
  default:
    os << "<unknown type>\n";
    return;
  }
}

///

// Check if n is a valid pointer and a node belonging to us
#define IS_OUR_NODE_VALID(n)                                                   \
  (((n).getPointer() != nullptr) && (((n).getInt() & idBitMask) != 0))

RandomPathSearcher::RandomPathSearcher(PForest &processForest, RNG &rng)
    : processForest{processForest}, theRNG{rng},
      idBitMask{processForest.getNextId()} {};

ExecutionState &RandomPathSearcher::selectState() {
  unsigned flips = 0, bits = 0, range = 0;
  PTreeNodePtr *root = nullptr;
  while (!root || !IS_OUR_NODE_VALID(*root))
    root = &processForest.getPTrees()
                .at(range++ % processForest.getPTrees().size() + 1)
                ->root;
  assert(root->getInt() & idBitMask && "Root should belong to the searcher");
  PTreeNode *n = root->getPointer();
  while (!n->state) {
    if (!IS_OUR_NODE_VALID(n->left)) {
      assert(IS_OUR_NODE_VALID(n->right) &&
             "Both left and right nodes invalid");
      assert(n != n->right.getPointer());
      n = n->right.getPointer();
    } else if (!IS_OUR_NODE_VALID(n->right)) {
      assert(IS_OUR_NODE_VALID(n->left) && "Both right and left nodes invalid");
      assert(n != n->left.getPointer());
      n = n->left.getPointer();
    } else {
      if (bits == 0) {
        flips = theRNG.getInt32();
        bits = 32;
      }
      --bits;
      n = ((flips & (1U << bits)) ? n->left : n->right).getPointer();
    }
  }

  return *n->state;
}

void RandomPathSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  // insert states
  for (auto &es : addedStates) {
    PTreeNode *pnode = es->ptreeNode, *parent = pnode->parent;
    PTreeNodePtr &root = processForest.getPTrees().at(pnode->getTreeID())->root;
    PTreeNodePtr *childPtr;

    childPtr = parent ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                              : &parent->right)
                      : &root;
    while (pnode && !IS_OUR_NODE_VALID(*childPtr)) {
      childPtr->setInt(childPtr->getInt() | idBitMask);
      pnode = parent;
      if (pnode)
        parent = pnode->parent;

      childPtr = parent
                     ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                             : &parent->right)
                     : &root;
    }
  }

  // remove states
  for (auto es : removedStates) {
    PTreeNode *pnode = es->ptreeNode, *parent = pnode->parent;
    PTreeNodePtr &root = processForest.getPTrees().at(pnode->getTreeID())->root;

    while (pnode && !IS_OUR_NODE_VALID(pnode->left) &&
           !IS_OUR_NODE_VALID(pnode->right)) {
      auto childPtr =
          parent ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                         : &parent->right)
                 : &root;
      assert(IS_OUR_NODE_VALID(*childPtr) && "Removing pTree child not ours");
      childPtr->setInt(childPtr->getInt() & ~idBitMask);
      pnode = parent;
      if (pnode)
        parent = pnode->parent;
    }
  }
}

bool RandomPathSearcher::empty() {
  bool res = true;
  for (const auto &ntree : processForest.getPTrees())
    res = res && !IS_OUR_NODE_VALID(ntree.second->root);
  return res;
}

void RandomPathSearcher::printName(llvm::raw_ostream &os) {
  os << "RandomPathSearcher\n";
}

///

BatchingSearcher::BatchingSearcher(Searcher *baseSearcher,
                                   time::Span timeBudget,
                                   unsigned instructionBudget)
    : baseSearcher{baseSearcher}, timeBudget{timeBudget},
      instructionBudget{instructionBudget} {};

ExecutionState &BatchingSearcher::selectState() {
  if (!lastState ||
      (((timeBudget.toSeconds() > 0) &&
        (time::getWallTime() - lastStartTime) > timeBudget)) ||
      ((instructionBudget > 0) &&
       (stats::instructions - lastStartInstructions) > instructionBudget)) {
    if (lastState) {
      time::Span delta = time::getWallTime() - lastStartTime;
      auto t = timeBudget;
      t *= 1.1;
      if (delta > t) {
        klee_message("increased time budget from %f to %f\n",
                     timeBudget.toSeconds(), delta.toSeconds());
        timeBudget = delta;
      }
    }
    lastState = &baseSearcher->selectState();
    lastStartTime = time::getWallTime();
    lastStartInstructions = stats::instructions;
    return *lastState;
  } else {
    return *lastState;
  }
}

void BatchingSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  // drop memoized state if it is marked for deletion
  if (std::find(removedStates.begin(), removedStates.end(), lastState) !=
      removedStates.end())
    lastState = nullptr;
  // update underlying searcher
  baseSearcher->update(current, addedStates, removedStates);
}

bool BatchingSearcher::empty() { return baseSearcher->empty(); }

void BatchingSearcher::printName(llvm::raw_ostream &os) {
  os << "<BatchingSearcher> timeBudget: " << timeBudget
     << ", instructionBudget: " << instructionBudget << ", baseSearcher:\n";
  baseSearcher->printName(os);
  os << "</BatchingSearcher>\n";
}

///

IterativeDeepeningTimeSearcher::IterativeDeepeningTimeSearcher(
    Searcher *baseSearcher)
    : baseSearcher{baseSearcher} {};

ExecutionState &IterativeDeepeningTimeSearcher::selectState() {
  ExecutionState &res = baseSearcher->selectState();
  startTime = time::getWallTime();
  return res;
}

void IterativeDeepeningTimeSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {

  const auto elapsed = time::getWallTime() - startTime;

  // update underlying searcher (filter paused states unknown to underlying
  // searcher)
  if (!removedStates.empty()) {
    std::vector<ExecutionState *> alt = removedStates;
    for (const auto state : removedStates) {
      auto it = pausedStates.find(state);
      if (it != pausedStates.end()) {
        pausedStates.erase(it);
        alt.erase(std::remove(alt.begin(), alt.end(), state), alt.end());
      }
    }
    baseSearcher->update(current, addedStates, alt);
  } else {
    baseSearcher->update(current, addedStates, removedStates);
  }

  // update current: pause if time exceeded
  if (current &&
      std::find(removedStates.begin(), removedStates.end(), current) ==
          removedStates.end() &&
      elapsed > time) {
    pausedStates.insert(current);
    baseSearcher->update(nullptr, {}, {current});
  }

  // no states left in underlying searcher: fill with paused states
  if (baseSearcher->empty()) {
    time *= 2U;
    klee_message("increased time budget to %f\n", time.toSeconds());
    std::vector<ExecutionState *> ps(pausedStates.begin(), pausedStates.end());
    baseSearcher->update(nullptr, ps, std::vector<ExecutionState *>());
    pausedStates.clear();
  }
}

bool IterativeDeepeningTimeSearcher::empty() {
  return baseSearcher->empty() && pausedStates.empty();
}

void IterativeDeepeningTimeSearcher::printName(llvm::raw_ostream &os) {
  os << "IterativeDeepeningTimeSearcher\n";
}

///

InterleavedSearcher::InterleavedSearcher(
    const std::vector<Searcher *> &_searchers) {
  searchers.reserve(_searchers.size());
  for (auto searcher : _searchers)
    searchers.emplace_back(searcher);
}

ExecutionState &InterleavedSearcher::selectState() {
  Searcher *s = searchers[--index].get();
  if (index == 0)
    index = searchers.size();
  return s->selectState();
}

void InterleavedSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {

  // update underlying searchers
  for (auto &searcher : searchers)
    searcher->update(current, addedStates, removedStates);
}

bool InterleavedSearcher::empty() { return searchers[0]->empty(); }

void InterleavedSearcher::printName(llvm::raw_ostream &os) {
  os << "<InterleavedSearcher> containing " << searchers.size()
     << " searchers:\n";
  for (const auto &searcher : searchers)
    searcher->printName(os);
  os << "</InterleavedSearcher>\n";
}
