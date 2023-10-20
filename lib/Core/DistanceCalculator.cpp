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
#include "klee/Module/CodeGraphInfo.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/Target.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/CFG.h"
#include "llvm/IR/IntrinsicInst.h"
DISABLE_WARNING_POP

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

unsigned DistanceCalculator::SpeculativeState::computeHash() {
  unsigned res =
      (reinterpret_cast<uintptr_t>(kb) * SymbolicSource::MAGIC_HASH_CONSTANT) +
      kind;
  hashValue = res;
  return hashValue;
}

DistanceResult DistanceCalculator::getDistance(const ExecutionState &state,
                                               KBlock *target) {
  return getDistance(state.prevPC, state.pc, state.stack.callStack(), target);
}

DistanceResult DistanceCalculator::getDistance(KBlock *kb, TargetKind kind,
                                               KBlock *target) {
  SpeculativeState specState(kb, kind);
  auto it1 = distanceResultCache.find(target);
  if (it1 == distanceResultCache.end()) {
    SpeculativeStateToDistanceResultMap m;
    it1 = distanceResultCache.emplace(target, std::move(m)).first;
  }
  auto &m = it1->second;
  auto it2 = m.find(specState);
  if (it2 == m.end()) {
    auto result = computeDistance(kb, kind, target);
    m.emplace(specState, result);
    return result;
  }
  return it2->second;
}

DistanceResult DistanceCalculator::computeDistance(KBlock *kb, TargetKind kind,
                                                   KBlock *target) const {
  weight_type weight = 0;
  WeightResult res = Miss;
  bool isInsideFunction = true;
  switch (kind) {
  case LocalTarget:
    res = tryGetTargetWeight(kb, weight, target);
    break;

  case PreTarget:
    res = tryGetPreTargetWeight(kb, weight, target);
    isInsideFunction = false;
    break;

  case PostTarget:
    res = tryGetPostTargetWeight(kb, weight, target);
    isInsideFunction = false;
    break;

  case NoneTarget:
    break;
  }
  return DistanceResult(res, weight, isInsideFunction);
}

DistanceResult DistanceCalculator::getDistance(
    const KInstruction *prevPC, const KInstruction *pc,
    const ExecutionStack::call_stack_ty &frames, KBlock *target) {
  KBlock *kb = pc->parent;
  const auto &distanceToTargetFunction =
      codeGraphInfo.getBackwardDistance(target->parent);
  unsigned int minCallWeight = UINT_MAX, minSfNum = UINT_MAX, sfNum = 0;
  auto sfi = frames.rbegin(), sfe = frames.rend();
  bool strictlyAfterKB =
      sfi != sfe && sfi->kf->parent->inMainModule(*sfi->kf->function);
  for (; sfi != sfe; sfi++, sfNum++) {
    unsigned callWeight;
    if (distanceInCallGraph(sfi->kf, kb, callWeight, distanceToTargetFunction,
                            target, strictlyAfterKB && sfNum != 0)) {
      callWeight = 2 * callWeight + sfNum;

      if (callWeight < UINT_MAX) {
        minCallWeight = callWeight;
        minSfNum = sfNum;
        break;
      }
    }

    if (sfi->caller)
      kb = sfi->caller->parent;
  }

  TargetKind kind = NoneTarget;
  if (minCallWeight == 0) {
    kind = LocalTarget;
  } else if (minSfNum == 0) {
    kind = PreTarget;
  } else if (minSfNum != UINT_MAX) {
    kind = PostTarget;
  }

  return getDistance(pc->parent, kind, target);
}

bool DistanceCalculator::distanceInCallGraph(
    KFunction *kf, KBlock *origKB, unsigned int &distance,
    const FunctionDistanceMap &distanceToTargetFunction, KBlock *targetKB,
    bool strictlyAfterKB) const {
  auto &dist = codeGraphInfo.getDistance(origKB->basicBlock);
  if (kf == targetKB->parent && dist.count(targetKB->basicBlock)) {
    distance = 0;
    return true;
  }

  distance = UINT_MAX;
  bool cannotReachItself = strictlyAfterKB && !codeGraphInfo.hasCycle(origKB);
  for (auto kCallBlock : kf->kCallBlocks) {
    if (!dist.count(kCallBlock->basicBlock) ||
        (cannotReachItself && origKB == kCallBlock))
      continue;
    for (auto calledFunction : kCallBlock->calledFunctions) {
      auto it = distanceToTargetFunction.find(calledFunction);
      if (it != distanceToTargetFunction.end() && distance > it->second + 1)
        distance = it->second + 1;
    }
  }
  return distance != UINT_MAX;
}

WeightResult DistanceCalculator::tryGetLocalWeight(
    KBlock *kb, weight_type &weight,
    const std::vector<KBlock *> &localTargets) const {
  const auto &dist = codeGraphInfo.getDistance(kb);
  weight = UINT_MAX;
  for (auto end : localTargets) {
    auto it = dist.find(end->basicBlock);
    if (it != dist.end())
      weight = std::min(it->second, weight);
  }

  if (weight == UINT_MAX)
    return Miss;
  if (weight == 0) {
    return Done;
  }

  return Continue;
}

WeightResult DistanceCalculator::tryGetPreTargetWeight(KBlock *kb,
                                                       weight_type &weight,
                                                       KBlock *target) const {
  auto &distanceToTargetFunction =
      codeGraphInfo.getBackwardDistance(target->parent);
  KFunction *currentKF = kb->parent;
  std::vector<KBlock *> localTargets;
  for (auto kCallBlock : currentKF->kCallBlocks) {
    for (auto calledFunction : kCallBlock->calledFunctions) {
      if (distanceToTargetFunction.count(calledFunction)) {
        localTargets.push_back(kCallBlock);
        break;
      }
    }
  }

  if (localTargets.empty())
    return Miss;

  WeightResult res = tryGetLocalWeight(kb, weight, localTargets);
  return res == Done ? Continue : res;
}

WeightResult DistanceCalculator::tryGetPostTargetWeight(KBlock *kb,
                                                        weight_type &weight,
                                                        KBlock *target) const {
  KFunction *currentKF = kb->parent;
  std::vector<KBlock *> &localTargets = currentKF->returnKBlocks;

  if (localTargets.empty())
    return Miss;

  WeightResult res = tryGetLocalWeight(kb, weight, localTargets);
  return res == Done ? Continue : res;
}

WeightResult DistanceCalculator::tryGetTargetWeight(KBlock *kb,
                                                    weight_type &weight,
                                                    KBlock *target) const {
  std::vector<KBlock *> localTargets = {target};
  WeightResult res = tryGetLocalWeight(kb, weight, localTargets);
  return res;
}
