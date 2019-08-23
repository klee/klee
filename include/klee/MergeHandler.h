//===-- MergeHandler.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/** 
 * @file MergeHandler.h
 * @brief Implementation of the region based merging
 *
 * ## Basic usage:
 * 
 * @code{.cpp}
 * klee_open_merge();
 * 
 * code containing branches etc. 
 * 
 * klee_close_merge();
 * @endcode
 * 
 * Will lead to all states that forked from the state that executed the
 * klee_open_merge() being merged in the klee_close_merge(). This allows for
 * fine-grained regions to be specified for merging.
 * 
 * # Implementation Structure
 * 
 * The main part of the new functionality is implemented in the class
 * klee::MergeHandler. The Special Function Handler generates an instance of
 * this class every time a state runs into a klee_open_merge() call.
 * 
 * This instance is appended to a `std::vector<klee::ref<klee::MergeHandler>>`
 * in the ExecutionState that passed the merge open point. This stack is also
 * copied during forks. We use a stack instead of a single instance to support
 * nested merge regions.
 * 
 * Once a state runs into a `klee_close_merge()`, the Special Function Handler
 * notifies the top klee::MergeHandler in the state's stack, pauses the state
 * from scheduling, and tries to merge it with all other states that already
 * arrived at the same close merge point. This top instance is then popped from
 * the stack, resulting in a decrease of the ref count of the
 * klee::MergeHandler.
 * 
 * Since the only references to this MergeHandler are in the stacks of
 * the ExecutionStates currently in the merging region, once the ref count
 * reaches zero, every state which ran into the same `klee_open_merge()` is now
 * paused and waiting to be merged. The destructor of the MergeHandler
 * then continues the scheduling of the corresponding paused states.
 *
 * # Non-blocking State Merging
 *
 * This feature adds functionality to the BoundedMergingSearcher that will
 * prioritize (e.g., immediately schedule) states running inside a bounded-merge
 * region once a state has reached a corresponding klee_close_merge() call. The
 * goal is to quickly gather all states inside the merging region in order to
 * release the waiting states. However, states that are running for more than
 * twice the mean number of instructions compared to the states that are already
 * waiting, will not be prioritized anymore.
 *
 * Once no more states are available for prioritizing, but there are states
 * waiting to be released, these states (which have already been merged as good as
 * possible) will be continued without waiting for the remaining states. When a
 * remaining state now enters a close-merge point, it will again wait for the
 * other states, or until the 'timeout' is reached.
*/

#ifndef KLEE_MERGEHANDLER_H
#define KLEE_MERGEHANDLER_H

#include <vector>
#include <map>
#include <stdint.h>
#include "llvm/Support/CommandLine.h"

namespace llvm {
class Instruction;
}

namespace klee {
extern llvm::cl::opt<bool> UseMerge;

extern llvm::cl::opt<bool> DebugLogMerge;

extern llvm::cl::opt<bool> DebugLogIncompleteMerge;

class Executor;
class ExecutionState;

/// @brief Represents one `klee_open_merge()` call. 
/// Handles merging of states that branched from it
class MergeHandler {
private:
  Executor *executor;

  /// @brief The instruction count when the state ran into the klee_open_merge
  uint64_t openInstruction;

  /// @brief The average number of instructions between the open and close merge of each
  /// state that has finished so far
  double closedMean;

  /// @brief Number of states that are tracked by this MergeHandler, that ran
  /// into a relevant klee_close_merge
  unsigned closedStateCount;

  /// @brief Get distance of state from the openInstruction
  unsigned getInstructionDistance(ExecutionState *es);

  /// @brief States that ran through the klee_open_merge, but not yet into a
  /// corresponding klee_close_merge
  std::vector<ExecutionState *> openStates;

  /// @brief Mapping the different 'klee_close_merge' calls to the states that ran into
  /// them
  std::map<llvm::Instruction *, std::vector<ExecutionState *> >
      reachedCloseMerge;

public:

  /// @brief Called when a state runs into a 'klee_close_merge()' call
  void addClosedState(ExecutionState *es, llvm::Instruction *mp);

  /// @brief Return state that should be prioritized to complete this merge
  ExecutionState *getPrioritizeState();

  /// @brief Add state to the 'openStates' vector
  void addOpenState(ExecutionState *es);

  /// @brief Remove state from the 'openStates' vector
  void removeOpenState(ExecutionState *es);

  /// @brief True, if any states have run into 'klee_close_merge()' and have
  /// not been released yet
  bool hasMergedStates();
  
  /// @brief Immediately release the merged states that have run into a
  /// 'klee_merge_close()'
  void releaseStates();

  // Return the mean time it takes for a state to get from klee_open_merge to
  // klee_close_merge
  double getMean();

  /// @brief Required by klee::ref objects
  unsigned refCount;


  MergeHandler(Executor *_executor, ExecutionState *es);
  ~MergeHandler();
};
}

#endif	/* KLEE_MERGEHANDLER_H */
