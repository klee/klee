#pragma once

#include <set>
#include <vector>
#include <map>
#include <stdint.h>

namespace llvm {
class Instruction;
}

namespace klee {
class Executor;
class ExecutionState;

class BoundedMergeHandler {
private:
  Executor *executor;

  // The instruction count when the state ran into the klee_open_merge
  uint64_t openInstruction;

  // The average number of instructions between the open and close merge of each
  // state that has finished yet
  double closeMean;

  // Number of states that are tracked by this BoundedMergeHandler, that ran
  // into a relevant klee_close_merge
  unsigned closedStateCount;

  // Get distance of state from the openInstruction
  unsigned getInstrDistance(ExecutionState *es);

  // States that ran through the klee_open_merge, but not yet into a
  // corresponding klee_close_merge
  std::vector<ExecutionState *> openStates;

  // Mapping the different 'klee_close_merge' calls to the states that ran into
  // them
  std::map<llvm::Instruction *, std::vector<ExecutionState *> >
      reachedMergeClose;

public:

  void addClosedState(ExecutionState *es, llvm::Instruction *mp);
  ExecutionState *getPrioritizeState();

  void addOpenState(ExecutionState *es);
  void removeOpenState(ExecutionState *es);
  void removeFromCloseMergeSet(ExecutionState *es);
  bool hasMergedStates();
  void releaseStates();

  // Return the mean time it takes for a state to get from klee_open_merge to
  // klee_close_merge
  double getMean();

  // Required by klee::ref objects
  unsigned refCount;


  BoundedMergeHandler(Executor *_executor, ExecutionState *es);
  ~BoundedMergeHandler();
};
}
