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

using BlockDistanceMap = std::unordered_map<KBlock *, unsigned>;
using FunctionDistanceMap = std::unordered_map<KFunction *, unsigned>;
using SortedBlockDistances = std::vector<std::pair<KBlock *, unsigned>>;
using SortedFunctionDistances = std::vector<std::pair<KFunction *, unsigned>>;

class CodeGraphInfo {

  using blockToDistanceMap = std::unordered_map<KBlock *, BlockDistanceMap>;
  using blockDistanceList = std::unordered_map<KBlock *, SortedBlockDistances>;

  using functionToDistanceMap =
      std::unordered_map<KFunction *, FunctionDistanceMap>;
  using functionDistanceList =
      std::unordered_map<KFunction *, SortedFunctionDistances>;

  using functionBranchesSet =
      std::unordered_map<KFunction *, KBlockMap<std::set<unsigned>>>;

private:
  blockToDistanceMap blockDistance;
  blockToDistanceMap blockBackwardDistance;
  KBlockSet blockCycles;
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
  void calculateDistance(KBlock *bb);
  void calculateBackwardDistance(KBlock *bb);

  void calculateDistance(KFunction *f);
  void calculateBackwardDistance(KFunction *f);

  void calculateFunctionBranches(KFunction *kf);
  void calculateFunctionConditionalBranches(KFunction *kf);
  void calculateFunctionBlocks(KFunction *kf);

public:
  const BlockDistanceMap &getDistance(KBlock *b);
  const BlockDistanceMap &getBackwardDistance(KBlock *kb);
  bool hasCycle(KBlock *kb);

  const FunctionDistanceMap &getDistance(KFunction *kf);
  const FunctionDistanceMap &getBackwardDistance(KFunction *kf);

  void getNearestPredicateSatisfying(KBlock *from, KBlockPredicate predicate,
                                     KBlockSet &result);

  const KBlockMap<std::set<unsigned>> &getFunctionBranches(KFunction *kf);

  const KBlockMap<std::set<unsigned>> &
  getFunctionConditionalBranches(KFunction *kf);
  const KBlockMap<std::set<unsigned>> &getFunctionBlocks(KFunction *kf);
};

} // namespace klee

#endif /* KLEE_CODEGRAPHGDISTANCE_H */
