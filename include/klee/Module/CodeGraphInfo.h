//===-- CodeGraphInfo.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CODEGRAPHGDISTANCE_H
#define KLEE_CODEGRAPHGDISTANCE_H

#include "klee/Module/KModule.h"

#include <unordered_map>

namespace klee {

using Block = llvm::BasicBlock;
using BlockDistanceMap = std::unordered_map<Block *, unsigned>;
using FunctionDistanceMap = std::unordered_map<llvm::Function *, unsigned>;
using SortedBlockDistances = std::vector<std::pair<Block *, unsigned>>;
using SortedFunctionDistances =
    std::vector<std::pair<llvm::Function *, unsigned>>;

class CodeGraphInfo {

  using blockToDistanceMap = std::unordered_map<Block *, BlockDistanceMap>;
  using blockDistanceList = std::unordered_map<Block *, SortedBlockDistances>;

  using functionToDistanceMap =
      std::unordered_map<llvm::Function *, FunctionDistanceMap>;
  using functionDistanceList =
      std::unordered_map<llvm::Function *, SortedFunctionDistances>;

  using functionBranchesSet =
      std::unordered_map<KFunction *, std::map<KBlock *, std::set<unsigned>>>;

private:
  blockToDistanceMap blockDistance;
  blockToDistanceMap blockBackwardDistance;
  std::set<Block *> blockCycles;
  blockDistanceList blockSortedDistance;
  blockDistanceList blockSortedBackwardDistance;

  functionToDistanceMap functionDistance;
  functionToDistanceMap functionBackwardDistance;
  functionDistanceList functionSortedDistance;
  functionDistanceList functionSortedBackwardDistance;

  functionBranchesSet functionBranches;
  functionBranchesSet functionConditionalBranches;
  functionBranchesSet functionBlocks;

private:
  void calculateDistance(Block *bb);
  void calculateBackwardDistance(Block *bb);

  void calculateDistance(KFunction *f);
  void calculateBackwardDistance(KFunction *f);

  void calculateFunctionBranches(KFunction *kf);
  void calculateFunctionConditionalBranches(KFunction *kf);
  void calculateFunctionBlocks(KFunction *kf);

public:
  const BlockDistanceMap &getDistance(Block *b);
  const BlockDistanceMap &getDistance(KBlock *kb);
  const BlockDistanceMap &getBackwardDistance(KBlock *kb);
  bool hasCycle(KBlock *kb);

  const FunctionDistanceMap &getDistance(KFunction *kf);
  const FunctionDistanceMap &getBackwardDistance(KFunction *kf);

  void getNearestPredicateSatisfying(KBlock *from, KBlockPredicate predicate,
                                     std::set<KBlock *> &result);

  const std::map<KBlock *, std::set<unsigned>> &
  getFunctionBranches(KFunction *kf);
  const std::map<KBlock *, std::set<unsigned>> &
  getFunctionConditionalBranches(KFunction *kf);
  const std::map<KBlock *, std::set<unsigned>> &
  getFunctionBlocks(KFunction *kf);
};

} // namespace klee

#endif /* KLEE_CODEGRAPHGDISTANCE_H */
