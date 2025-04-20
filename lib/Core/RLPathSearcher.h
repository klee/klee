//===-- RLPathSearcher.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_RLPATHSEARCHER_H
#define KLEE_RLPATHSEARCHER_H

#include "Searcher.h"
#include "RLInterface.h"

namespace klee {

/// RLPathSearcher uses reinforcement learning to guide the search through
/// the execution tree by learning which paths are more likely to lead to
/// interesting states or coverage.
class RLPathSearcher final : public Searcher {
private:
  InMemoryExecutionTree *executionTree;
  RNG &theRNG;
  
  // RL interface for decision making
  RLInterface rlInterface;

  // Unique bitmask of this searcher (similar to RandomPathSearcher)
  const uint8_t idBitMask;
  
  // Last visited node for reward calculation
  ExecutionTreeNode* lastVisitedNode {nullptr};
  
  // Calculate reward based on state transitions
  float calculateReward(ExecutionTreeNode* node);

public:
  /// \param executionTree The execution tree.
  /// \param RNG A random number generator.
  RLPathSearcher(InMemoryExecutionTree *executionTree, RNG &rng);
  ~RLPathSearcher() override = default;

  ExecutionState &selectState() override;
  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

} // namespace klee

#endif /* KLEE_RLPATHSEARCHER_H */
