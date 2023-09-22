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

class CodeGraphInfo {

  using blockDistanceMap =
      std::unordered_map<KBlock *, std::unordered_map<KBlock *, unsigned>>;
  using blockDistanceList =
      std::unordered_map<KBlock *, std::vector<std::pair<KBlock *, unsigned>>>;

  using functionDistanceMap =
      std::unordered_map<KFunction *,
                         std::unordered_map<KFunction *, unsigned>>;
  using functionDistanceList =
      std::unordered_map<KFunction *,
                         std::vector<std::pair<KFunction *, unsigned>>>;

  using functionBranchesSet =
      std::unordered_map<KFunction *, std::map<KBlock *, std::set<unsigned>>>;

private:
  blockDistanceMap blockDistance;
  blockDistanceMap blockBackwardDistance;
  blockDistanceList blockSortedDistance;
  blockDistanceList blockSortedBackwardDistance;

  functionDistanceMap functionDistance;
  functionDistanceMap functionBackwardDistance;
  functionDistanceList functionSortedDistance;
  functionDistanceList functionSortedBackwardDistance;

  functionBranchesSet functionBranches;
  functionBranchesSet functionConditionalBranches;
  functionBranchesSet functionBlocks;

private:
  void calculateDistance(KBlock *bb);
  void calculateBackwardDistance(KBlock *bb);

  void calculateDistance(KFunction *kf);
  void calculateBackwardDistance(KFunction *kf);

  void calculateFunctionBranches(KFunction *kf);
  void calculateFunctionConditionalBranches(KFunction *kf);
  void calculateFunctionBlocks(KFunction *kf);

public:
  const std::unordered_map<KBlock *, unsigned int> &getDistance(KBlock *kb);
  const std::unordered_map<KBlock *, unsigned int> &
  getBackwardDistance(KBlock *kb);
  const std::vector<std::pair<KBlock *, unsigned int>> &
  getSortedDistance(KBlock *kb);
  const std::vector<std::pair<KBlock *, unsigned int>> &
  getSortedBackwardDistance(KBlock *kb);

  const std::unordered_map<KFunction *, unsigned int> &
  getDistance(KFunction *kf);
  const std::unordered_map<KFunction *, unsigned int> &
  getBackwardDistance(KFunction *kf);
  const std::vector<std::pair<KFunction *, unsigned int>> &
  getSortedDistance(KFunction *kf);
  const std::vector<std::pair<KFunction *, unsigned int>> &
  getSortedBackwardDistance(KFunction *kf);

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
