//===-- DistanceCalculator.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "DistanceCalculator.h"
#include "ExecutionState.h"
#include "klee/Module/CodeGraphDistance.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/Target.h"

#include <limits>

using namespace llvm;
using namespace klee;

bool DistanceResult::operator<(const DistanceResult &b) const {
  if (isInsideFunction != b.isInsideFunction)
    return isInsideFunction;
  if (result == WeightResult::Continue && b.result == WeightResult::Continue)
    return weight < b.weight;
  return result < b.result;
}

std::string DistanceResult::toString() const {
  std::ostringstream out;
  out << "(" << (int)!isInsideFunction << ", " << (int)result << ", " << weight
      << ")";
  return out.str();
}

DistanceResult DistanceCalculator::getDistance(ExecutionState &es,
                                               ref<Target> target) {
  return getDistance(es.pc, es.prevPC, es.initPC, es.stack, es.error, target);
}

DistanceResult
DistanceCalculator::getDistance(KInstruction *pc, KInstruction *prevPC,
                                KInstruction *initPC,
                                const ExecutionState::stack_ty &stack,
                                ReachWithError error, ref<Target> target) {
  weight_type weight = 0;

  BasicBlock *pcBlock = pc->inst->getParent();
  BasicBlock *prevPCBlock = prevPC->inst->getParent();

  if (!target->shouldFailOnThisTarget() && target->atReturn()) {
    if (prevPC->parent == target->getBlock() &&
        prevPC == target->getBlock()->getLastInstruction()) {
      return DistanceResult(Done);
    } else if (pc->parent == target->getBlock()) {
      return DistanceResult(Continue);
    }
  }

  if (target->shouldFailOnThisTarget() && target->isTheSameAsIn(prevPC) &&
      target->isThatError(error)) {
    return DistanceResult(Done);
  }

  BasicBlock *bb = pcBlock;
  KBlock *kb = pc->parent->parent->blockMap[bb];
  const auto &distanceToTargetFunction =
      codeGraphDistance.getBackwardDistance(target->getBlock()->parent);
  unsigned int minCallWeight = UINT_MAX, minSfNum = UINT_MAX, sfNum = 0;
  for (auto sfi = stack.rbegin(), sfe = stack.rend(); sfi != sfe; sfi++) {
    unsigned callWeight;
    if (distanceInCallGraph(sfi->kf, kb, callWeight, distanceToTargetFunction,
                            target)) {
      callWeight *= 2;
      callWeight += sfNum;

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
  bool isInsideFunction = true;
  if (minCallWeight == 0) {
    res = tryGetTargetWeight(pc, initPC, pcBlock, prevPCBlock, weight, target);
  } else if (minSfNum == 0) {
    res = tryGetPreTargetWeight(pc, initPC, pcBlock, prevPCBlock, weight,
                                distanceToTargetFunction, target);
    isInsideFunction = false;
  } else if (minSfNum != UINT_MAX) {
    res = tryGetPostTargetWeight(pc, initPC, pcBlock, prevPCBlock, weight,
                                 target);
    isInsideFunction = false;
  }
  if (Done == res && target->shouldFailOnThisTarget()) {
    if (!target->isThatError(error)) {
      res = Continue;
    }
  }

  return DistanceResult(res, weight, isInsideFunction);
}

bool DistanceCalculator::distanceInCallGraph(
    KFunction *kf, KBlock *kb, unsigned int &distance,
    const std::unordered_map<KFunction *, unsigned int>
        &distanceToTargetFunction,
    ref<Target> target) {
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

WeightResult DistanceCalculator::tryGetLocalWeight(
    KInstruction *pc, KInstruction *initPC, BasicBlock *pcBlock,
    BasicBlock *prevPCBlock, weight_type &weight,
    const std::vector<KBlock *> &localTargets, ref<Target> target) {
  KFunction *currentKF = pc->parent->parent;
  KBlock *initKB = initPC->parent;
  KBlock *currentKB = currentKF->blockMap[pcBlock];
  KBlock *prevKB = currentKF->blockMap[prevPCBlock];
  const std::unordered_map<KBlock *, unsigned> &dist =
      codeGraphDistance.getDistance(currentKB);
  weight = UINT_MAX;
  for (auto &end : localTargets) {
    if (dist.count(end) > 0) {
      unsigned int w = dist.at(end);
      weight = std::min(w, weight);
    }
  }

  if (weight == UINT_MAX)
    return Miss;
  if (weight == 0 && (initKB == currentKB || prevKB != currentKB ||
                      target->shouldFailOnThisTarget())) {
    return Done;
  }

  return Continue;
}

WeightResult DistanceCalculator::tryGetPreTargetWeight(
    KInstruction *pc, KInstruction *initPC, BasicBlock *pcBlock,
    BasicBlock *prevPCBlock, weight_type &weight,
    const std::unordered_map<KFunction *, unsigned int>
        &distanceToTargetFunction,
    ref<Target> target) {
  KFunction *currentKF = pc->parent->parent;
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

  WeightResult res = tryGetLocalWeight(pc, initPC, pcBlock, prevPCBlock, weight,
                                       localTargets, target);
  return res == Done ? Continue : res;
}

WeightResult DistanceCalculator::tryGetPostTargetWeight(
    KInstruction *pc, KInstruction *initPC, BasicBlock *pcBlock,
    BasicBlock *prevPCBlock, weight_type &weight, ref<Target> target) {
  KFunction *currentKF = pc->parent->parent;
  std::vector<KBlock *> &localTargets = currentKF->returnKBlocks;

  if (localTargets.empty())
    return Miss;

  WeightResult res = tryGetLocalWeight(pc, initPC, pcBlock, prevPCBlock, weight,
                                       localTargets, target);
  return res == Done ? Continue : res;
}

WeightResult DistanceCalculator::tryGetTargetWeight(
    KInstruction *pc, KInstruction *initPC, BasicBlock *pcBlock,
    BasicBlock *prevPCBlock, weight_type &weight, ref<Target> target) {
  std::vector<KBlock *> localTargets = {target->getBlock()};
  WeightResult res = tryGetLocalWeight(pc, initPC, pcBlock, prevPCBlock, weight,
                                       localTargets, target);
  return res;
}
