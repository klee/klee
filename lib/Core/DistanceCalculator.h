//===-- DistanceCalculator.h ------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_DISTANCE_CALCULATOR_H
#define KLEE_DISTANCE_CALCULATOR_H

#include "ExecutionState.h"

namespace llvm {
class BasicBlock;
} // namespace llvm

namespace klee {
class CodeGraphDistance;
struct Target;

enum WeightResult : std::uint8_t {
  Done = 0U,
  Continue = 1U,
  Miss = 2U,
};

using weight_type = unsigned;

struct DistanceResult {
  WeightResult result;
  weight_type weight;
  bool isInsideFunction;

  explicit DistanceResult(WeightResult result_ = WeightResult::Miss,
                          weight_type weight_ = 0,
                          bool isInsideFunction_ = true)
      : result(result_), weight(weight_), isInsideFunction(isInsideFunction_){};

  bool operator<(const DistanceResult &b) const;

  std::string toString() const;
};

class DistanceCalculator {
public:
  explicit DistanceCalculator(CodeGraphDistance &codeGraphDistance_)
      : codeGraphDistance(codeGraphDistance_) {}

  DistanceResult getDistance(ExecutionState &es, ref<Target> target);
  DistanceResult getDistance(KInstruction *pc, KInstruction *prevPC,
                             KInstruction *initPC,
                             const ExecutionState::stack_ty &stack,
                             ReachWithError error, ref<Target> target);

private:
  CodeGraphDistance &codeGraphDistance;

  bool distanceInCallGraph(KFunction *kf, KBlock *kb, unsigned int &distance,
                           const std::unordered_map<KFunction *, unsigned int>
                               &distanceToTargetFunction,
                           ref<Target> target);
  bool distanceInCallGraph(KFunction *kf, KBlock *kb, unsigned int &distance,
                           const std::unordered_map<KFunction *, unsigned int>
                               &distanceToTargetFunction,
                           ref<Target> target, bool strictlyAfterKB);
  WeightResult tryGetLocalWeight(KInstruction *pc, KInstruction *initPC,
                                 llvm::BasicBlock *pcBlock,
                                 llvm::BasicBlock *prevPCBlock,
                                 weight_type &weight,
                                 const std::vector<KBlock *> &localTargets,
                                 ref<Target> target);
  WeightResult
  tryGetPreTargetWeight(KInstruction *pc, KInstruction *initPC,
                        llvm::BasicBlock *pcBlock,
                        llvm::BasicBlock *prevPCBlock, weight_type &weight,
                        const std::unordered_map<KFunction *, unsigned int>
                            &distanceToTargetFunction,
                        ref<Target> target);
  WeightResult tryGetTargetWeight(KInstruction *pc, KInstruction *initPC,
                                  llvm::BasicBlock *pcBlock,
                                  llvm::BasicBlock *prevPCBlock,
                                  weight_type &weight, ref<Target> target);
  WeightResult tryGetPostTargetWeight(KInstruction *pc, KInstruction *initPC,
                                      llvm::BasicBlock *pcBlock,
                                      llvm::BasicBlock *prevPCBlock,
                                      weight_type &weight, ref<Target> target);
};
} // namespace klee

#endif /* KLEE_DISTANCE_CALCULATOR_H */
