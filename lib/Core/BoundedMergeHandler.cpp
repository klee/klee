#include "klee/BoundedMergeHandler.h"

#include "CoreStats.h"
#include "Executor.h"
#include "klee/ExecutionState.h"

namespace klee {

double BoundedMergeHandler::getMean() {
  if (closedStateCount == 0)
    return 0;

  return closeMean;
}

unsigned BoundedMergeHandler::getInstrDistance(ExecutionState *es){
  return es->steppedInstructions - openInstruction;
}

ExecutionState *BoundedMergeHandler::getPrioritizeState(){
  for (std::vector<ExecutionState *>::iterator
           state_it = openStates.begin(),
           state_ie = openStates.end();
       state_it != state_ie; ++state_it) {
    bool stateIsClosed = (executor->inCloseMerge.find(*state_it) !=
                          executor->inCloseMerge.end());

    //llvm::errs() << "prioritize: " << state << " " << stateIsClosed
      if (!stateIsClosed &&
        (getInstrDistance(*state_it) < 2 * getMean())) {
      return *state_it; 
    }
  }
  return 0;
}


void BoundedMergeHandler::addOpenState(ExecutionState *es){
  openStates.push_back(es);
}

void BoundedMergeHandler::removeOpenState(ExecutionState *es){
  std::vector<ExecutionState *>::iterator it =
      std::find(openStates.begin(), openStates.end(), es);
  assert(it != openStates.end());
  std::swap(*it, openStates.back());
  openStates.pop_back();
}

void BoundedMergeHandler::removeFromCloseMergeSet(ExecutionState *es){
  executor->inCloseMerge.erase(es);
}

void BoundedMergeHandler::addClosedState(ExecutionState *es,
                                         llvm::Instruction *mp) {
  // Update stats
  ++closedStateCount;
  closeMean += (static_cast<double>(getInstrDistance(es)) - closeMean) /
               closedStateCount;

  // Remove from openStates
  removeOpenState(es);

  std::map<llvm::Instruction *, std::vector<ExecutionState *> >::iterator
      closePoint = reachedMergeClose.find(mp);

  // If no other state has yet encountered this klee_close_merge instruction,
  // add a new element to the map
  if (closePoint == reachedMergeClose.end()) {
    reachedMergeClose[mp].push_back(es);
    executor->pauseState(*es);
  } else {
    // Otherwise try to merge with any state in the map element for this
    // instruction
    std::vector<ExecutionState *> &cpv = closePoint->second;
    bool mergedSuccessful = false;

    for (std::vector<ExecutionState *>::iterator it = cpv.begin(),
                                                 ie = cpv.end();
         it != ie; ++it) {
      if ((*it)->merge(*es)) {
        executor->terminateState(*es);
        mergedSuccessful = true;
        break;
      }
    }
    if (!mergedSuccessful) {
      cpv.push_back(es);
      executor->pauseState(*es);
    }
  }
}

void BoundedMergeHandler::releaseStates() {
  for (std::map<llvm::Instruction *, std::vector<ExecutionState *> >::iterator
           it = reachedMergeClose.begin(),
           ie = reachedMergeClose.end();
       it != ie; ++it) {
    for (std::vector<ExecutionState *>::iterator it2 = it->second.begin(),
                                                 ie2 = it->second.end();
         it2 != ie2; ++it2) {
      executor->continueState(**it2);
      removeFromCloseMergeSet(*it2);
    }
  }
  closedStateCount = 0;
  closeMean = 0;
  reachedMergeClose.clear();
}

bool BoundedMergeHandler::hasMergedStates() {
  return (!reachedMergeClose.empty());
}

BoundedMergeHandler::BoundedMergeHandler(Executor *_executor,
                                         ExecutionState *es)
    : executor(_executor), openInstruction(es->steppedInstructions),
      closedStateCount(0), refCount(0) {
  executor->mergeGroups.push_back(this);
  addOpenState(es);
}

BoundedMergeHandler::~BoundedMergeHandler() {
  std::vector<BoundedMergeHandler *>::iterator it =
      std::find(executor->mergeGroups.begin(), executor->mergeGroups.end(), this);
  std::swap(*it, executor->mergeGroups.back());
executor->mergeGroups.pop_back();

  releaseStates();
}
}
