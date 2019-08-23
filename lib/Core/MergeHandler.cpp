//===-- MergeHandler.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/MergeHandler.h"

#include "CoreStats.h"
#include "Executor.h"
#include "Searcher.h"
#include "klee/ExecutionState.h"

namespace klee {

/*** Test generation options ***/

llvm::cl::OptionCategory MergeCat("Path merging options",
                                  "These options control path merging.");

llvm::cl::opt<bool> UseMerge(
    "use-merge", llvm::cl::init(false),
    llvm::cl::desc("Enable support for path merging via klee_open_merge() and "
                   "klee_close_merge() (default=false)"),
    llvm::cl::cat(klee::MergeCat));

llvm::cl::opt<bool> DebugLogMerge(
    "debug-log-merge", llvm::cl::init(false),
    llvm::cl::desc("Debug information for path merging (default=false)"),
    llvm::cl::cat(klee::MergeCat));

llvm::cl::opt<bool> UseIncompleteMerge(
    "use-incomplete-merge", llvm::cl::init(false),
    llvm::cl::desc("Heuristic-based path merging (default=false)"),
    llvm::cl::cat(klee::MergeCat));

llvm::cl::opt<bool> DebugLogIncompleteMerge(
    "debug-log-incomplete-merge", llvm::cl::init(false),
    llvm::cl::desc("Debug information for incomplete path merging (default=false)"),
    llvm::cl::cat(klee::MergeCat));

double MergeHandler::getMean() {
  if (closedStateCount == 0)
    return 0;

  return closedMean;
}

unsigned MergeHandler::getInstructionDistance(ExecutionState *es){
  return es->steppedInstructions - openInstruction;
}

ExecutionState *MergeHandler::getPrioritizeState(){
  for (ExecutionState *cur_state : openStates) {
    bool stateIsClosed =
        (executor->mergingSearcher->inCloseMerge.find(cur_state) !=
         executor->mergingSearcher->inCloseMerge.end());

    if (!stateIsClosed && (getInstructionDistance(cur_state) < 2 * getMean())) {
      return cur_state;
    }
  }
  return 0;
}


void MergeHandler::addOpenState(ExecutionState *es){
  openStates.push_back(es);
}

void MergeHandler::removeOpenState(ExecutionState *es){
  auto it = std::find(openStates.begin(), openStates.end(), es);
  assert(it != openStates.end());
  std::swap(*it, openStates.back());
  openStates.pop_back();
}

void MergeHandler::addClosedState(ExecutionState *es,
                                         llvm::Instruction *mp) {
  // Update stats
  ++closedStateCount;

  // Incremental update of mean (travelling mean)
  // https://math.stackexchange.com/a/1836447
  closedMean += (static_cast<double>(getInstructionDistance(es)) - closedMean) /
               closedStateCount;

  // Remove from openStates
  removeOpenState(es);

  auto closePoint = reachedCloseMerge.find(mp);

  // If no other state has yet encountered this klee_close_merge instruction,
  // add a new element to the map
  if (closePoint == reachedCloseMerge.end()) {
    reachedCloseMerge[mp].push_back(es);
    executor->mergingSearcher->pauseState(*es);
  } else {
    // Otherwise try to merge with any state in the map element for this
    // instruction
    auto &cpv = closePoint->second;
    bool mergedSuccessful = false;

    for (auto& mState: cpv) {
      if (mState->merge(*es)) {
        executor->terminateState(*es);
        executor->mergingSearcher->inCloseMerge.erase(es);
        mergedSuccessful = true;
        break;
      }
    }
    if (!mergedSuccessful) {
      cpv.push_back(es);
      executor->mergingSearcher->pauseState(*es);
    }
  }
}

void MergeHandler::releaseStates() {
  for (auto& curMergeGroup: reachedCloseMerge) {
    for (auto curState: curMergeGroup.second) {
      executor->mergingSearcher->continueState(*curState);
      executor->mergingSearcher->inCloseMerge.erase(curState);
    }
  }
  reachedCloseMerge.clear();
}

bool MergeHandler::hasMergedStates() {
  return (!reachedCloseMerge.empty());
}

MergeHandler::MergeHandler(Executor *_executor, ExecutionState *es)
    : executor(_executor), openInstruction(es->steppedInstructions),
      closedMean(0), closedStateCount(0), refCount(0) {
  executor->mergingSearcher->mergeGroups.push_back(this);
  addOpenState(es);
}

MergeHandler::~MergeHandler() {
  auto it = std::find(executor->mergingSearcher->mergeGroups.begin(),
                      executor->mergingSearcher->mergeGroups.end(), this);
  assert(it != executor->mergingSearcher->mergeGroups.end() &&
         "All MergeHandlers should be registered in mergeGroups");
  std::swap(*it, executor->mergingSearcher->mergeGroups.back());
  executor->mergingSearcher->mergeGroups.pop_back();

  releaseStates();
}
}
