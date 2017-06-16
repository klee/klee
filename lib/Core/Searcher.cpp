//===-- Searcher.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Searcher.h"

#include "CoreStats.h"
#include "Executor.h"
#include "PTree.h"
#include "StatsTracker.h"
#include "klee/BoundedMergeHandler.h"

#include "klee/CommandLine.h"

#include "klee/ExecutionState.h"
#include "klee/Statistics.h"
#include "klee/Internal/Module/InstructionInfoTable.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/ADT/DiscretePDF.h"
#include "klee/Internal/ADT/RNG.h"
#include "klee/Internal/Support/ModuleUtil.h"
#include "klee/Internal/System/Time.h"
#include "klee/Internal/Support/ErrorHandling.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#else
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#endif
#include "llvm/Support/CommandLine.h"

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
#include "llvm/Support/CallSite.h"
#else
#include "llvm/IR/CallSite.h"
#endif

#include <cassert>
#include <fstream>
#include <climits>

using namespace klee;
using namespace llvm;

namespace {
  cl::opt<bool>
  DebugLogMerge("debug-log-merge");
  cl::opt<bool>
  IncompleteBoundedMerge("incomplete-bounded-merge");
}

namespace klee {
  extern RNG theRNG;
}

Searcher::~Searcher() {
}

///

ExecutionState &DFSSearcher::selectState() {
  return *states.back();
}

void DFSSearcher::update(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates) {
  states.insert(states.end(),
                addedStates.begin(),
                addedStates.end());
  for (std::vector<ExecutionState *>::const_iterator it = removedStates.begin(),
                                                     ie = removedStates.end();
       it != ie; ++it) {
    ExecutionState *es = *it;
    if (es == states.back()) {
      states.pop_back();
    } else {
      bool ok = false;

      for (std::vector<ExecutionState*>::iterator it = states.begin(),
             ie = states.end(); it != ie; ++it) {
        if (es==*it) {
          states.erase(it);
          ok = true;
          break;
        }
      }

      assert(ok && "invalid state removed");
    }
  }
}

///

ExecutionState &BFSSearcher::selectState() {
  return *states.front();
}

void BFSSearcher::update(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates) {
  // Assumption: If new states were added KLEE forked, therefore states evolved.
  // constraints were added to the current state, it evolved.
  if (!addedStates.empty() && current &&
      std::find(removedStates.begin(), removedStates.end(), current) ==
          removedStates.end()) {
    assert(states.front() == current);
    states.pop_front();
    states.push_back(current);
  }

  states.insert(states.end(),
                addedStates.begin(),
                addedStates.end());
  for (std::vector<ExecutionState *>::const_iterator it = removedStates.begin(),
                                                     ie = removedStates.end();
       it != ie; ++it) {
    ExecutionState *es = *it;
    if (es == states.front()) {
      states.pop_front();
    } else {
      bool ok = false;

      for (std::deque<ExecutionState*>::iterator it = states.begin(),
             ie = states.end(); it != ie; ++it) {
        if (es==*it) {
          states.erase(it);
          ok = true;
          break;
        }
      }

      assert(ok && "invalid state removed");
    }
  }
}

///

ExecutionState &RandomSearcher::selectState() {
  return *states[theRNG.getInt32()%states.size()];
}

void
RandomSearcher::update(ExecutionState *current,
                       const std::vector<ExecutionState *> &addedStates,
                       const std::vector<ExecutionState *> &removedStates) {
  states.insert(states.end(),
                addedStates.begin(),
                addedStates.end());
  for (std::vector<ExecutionState *>::const_iterator it = removedStates.begin(),
                                                     ie = removedStates.end();
       it != ie; ++it) {
    ExecutionState *es = *it;
    bool ok = false;

    for (std::vector<ExecutionState*>::iterator it = states.begin(),
           ie = states.end(); it != ie; ++it) {
      if (es==*it) {
        states.erase(it);
        ok = true;
        break;
      }
    }
    
    assert(ok && "invalid state removed");
  }
}

///

WeightedRandomSearcher::WeightedRandomSearcher(WeightType _type)
  : states(new DiscretePDF<ExecutionState*>()),
    type(_type) {
  switch(type) {
  case Depth: 
    updateWeights = false;
    break;
  case InstCount:
  case CPInstCount:
  case QueryCost:
  case MinDistToUncovered:
  case CoveringNew:
    updateWeights = true;
    break;
  default:
    assert(0 && "invalid weight type");
  }
}

WeightedRandomSearcher::~WeightedRandomSearcher() {
  delete states;
}

ExecutionState &WeightedRandomSearcher::selectState() {
  return *states->choose(theRNG.getDoubleL());
}

double WeightedRandomSearcher::getWeight(ExecutionState *es) {
  switch(type) {
  default:
  case Depth: 
    return es->weight;
  case InstCount: {
    uint64_t count = theStatisticManager->getIndexedValue(stats::instructions,
                                                          es->pc->info->id);
    double inv = 1. / std::max((uint64_t) 1, count);
    return inv * inv;
  }
  case CPInstCount: {
    StackFrame &sf = es->stack.back();
    uint64_t count = sf.callPathNode->statistics.getValue(stats::instructions);
    double inv = 1. / std::max((uint64_t) 1, count);
    return inv;
  }
  case QueryCost:
    return (es->queryCost < .1) ? 1. : 1./es->queryCost;
  case CoveringNew:
  case MinDistToUncovered: {
    uint64_t md2u = computeMinDistToUncovered(es->pc,
                                              es->stack.back().minDistToUncoveredOnReturn);

    double invMD2U = 1. / (md2u ? md2u : 10000);
    if (type==CoveringNew) {
      double invCovNew = 0.;
      if (es->instsSinceCovNew)
        invCovNew = 1. / std::max(1, (int) es->instsSinceCovNew - 1000);
      return (invCovNew * invCovNew + invMD2U * invMD2U);
    } else {
      return invMD2U * invMD2U;
    }
  }
  }
}

void WeightedRandomSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  if (current && updateWeights &&
      std::find(removedStates.begin(), removedStates.end(), current) ==
          removedStates.end())
    states->update(current, getWeight(current));

  for (std::vector<ExecutionState *>::const_iterator it = addedStates.begin(),
                                                     ie = addedStates.end();
       it != ie; ++it) {
    ExecutionState *es = *it;
    states->insert(es, getWeight(es));
  }

  for (std::vector<ExecutionState *>::const_iterator it = removedStates.begin(),
                                                     ie = removedStates.end();
       it != ie; ++it) {
    states->remove(*it);
  }
}

bool WeightedRandomSearcher::empty() { 
  return states->empty(); 
}

///
RandomPathSearcher::BacktrackHelper::direction RandomPathSearcher::BacktrackHelper::avail_dir(){
  switch(visited.back()){
    case LEFT:
      return RIGHT;
    case RIGHT:
      return LEFT;
    case BOTH:
      return NONE;
    case NONE:
      return BOTH;
    default:
      assert(0);
  }
}

void RandomPathSearcher::BacktrackHelper::back(){
  assert(!visited.empty());
  visited.pop_back();   
}

void RandomPathSearcher::BacktrackHelper::go(RandomPathSearcher::BacktrackHelper::direction dir){
  switch (visited.back()){
    case LEFT:
      assert(dir == RIGHT);
      visited.back() = BOTH;
      break;
    case RIGHT:
      assert(dir == LEFT);
      visited.back() = BOTH;
      break;
    case NONE:
      switch(dir){
        case LEFT:
          visited.back() = LEFT;
          break;
        case RIGHT:
          visited.back() = RIGHT;
          break;
        default:
          assert(0 && "Not a correct go direction");
      }
      break;
    case BOTH:
      assert(0 && "No direction to go to");
      break;
  }
  visited.push_back(NONE);
}

RandomPathSearcher::RandomPathSearcher(Executor &_executor)
  : executor(_executor) {
}

RandomPathSearcher::~RandomPathSearcher() {
}

ExecutionState &RandomPathSearcher::selectStateIgnore() {
  unsigned flips=0, bits=0;
  PTree::Node* n = executor.processTree->root;
  BacktrackHelper btHelper;

  while (!n->data) {
    BacktrackHelper::direction avail_dir = btHelper.avail_dir();
    switch(avail_dir){
      case BacktrackHelper::RIGHT:
        if (n->right) {
          btHelper.go(BacktrackHelper::RIGHT);
          n = n->right;
        } else {
          btHelper.back();
          n = n->parent;
        }
        break;
      case BacktrackHelper::LEFT:
        if (n->left){
          btHelper.go(BacktrackHelper::LEFT);
          n = n->left;
        } else {
          btHelper.back();
          n = n->parent;
        }
        break;
      case BacktrackHelper::BOTH:
        if (!n->left){
          btHelper.go(BacktrackHelper::RIGHT);
          n = n->right;
        } else if (!n->right){
          btHelper.go(BacktrackHelper::LEFT);
          n = n->left;
        } else {
          if (bits==0) {
            flips = theRNG.getInt32();
            bits = 32;
          }
          --bits;
          if (flips&(1<<bits)){
            btHelper.go(BacktrackHelper::LEFT);
            n = n->left;
          } else {  
            btHelper.go(BacktrackHelper::RIGHT);
            n = n->right;
          }
        }
        break;
      case BacktrackHelper::NONE:
        btHelper.back();
        n = n->parent;
    }
    if (n->data){
      if (ignoreStates.find(n->data) != ignoreStates.end()){
        btHelper.back();
        n = n->parent;
      }
    }
  }
  return *n->data;
}
ExecutionState &RandomPathSearcher::selectStateStandard() {
  unsigned flips=0, bits=0;
  PTree::Node* n = executor.processTree->root;
  BacktrackHelper btHelper;
  while (!n->data) {
    if (!n->left) {
      n = n->right;
    } else if (!n->right){
      n = n->left;
    } else {
      if (bits==0) {
        flips = theRNG.getInt32();
        bits = 32;
      }
      --bits;
      if (flips&(1<<bits)){
        n = n->left;
      } else {  
        n = n->right;
      }
    }
  }

  return *(n->data);
}

ExecutionState& RandomPathSearcher::selectState() {
  if (ignoreStates.empty()){
    return selectStateStandard();
  } else {
    return selectStateIgnore();
  }
}

void
RandomPathSearcher::update(ExecutionState *current,
                           const std::vector<ExecutionState *> &addedStates,
                           const std::vector<ExecutionState *> &removedStates) {
  if ((&addedStates == &executor.addedStates) &&
      (&removedStates == &executor.removedStates))
    for (std::vector<ExecutionState *>::const_iterator
             it = removedStates.begin(),
             ie = removedStates.end();
         it != ie; ++it) {
      std::set<ExecutionState *>::iterator ignore_it = ignoreStates.find(*it);
      if (ignore_it != ignoreStates.end()) {
        ignoreStates.erase(ignore_it);
      }
    }

  ignoreStates.insert(removedStates.begin(), removedStates.end());
  if (!addedStates.empty()){
    std::set<ExecutionState*> new_is;
    for (std::set<ExecutionState *>::iterator it = ignoreStates.begin(),
                                              ie = ignoreStates.end();
         it != ie; ++it) {
      if (std::find(addedStates.begin(), addedStates.end(), *it) == addedStates.end()){
        new_is.insert(*it);
      }
    }
    ignoreStates = new_is; // cannot use std::move(new_is) because that is a c++11 feature
  }
}

bool RandomPathSearcher::empty() { 
  return executor.states.empty(); 
}

///

BoundedMergingSearcher::BoundedMergingSearcher(Executor &_executor, Searcher *_baseSearcher)
  : executor(_executor),
  baseSearcher(_baseSearcher),
  openMergeFunction(_executor.kmodule->kleeOpenMergeFn),
  closeMergeFunction(_executor.kmodule->kleeCloseMergeFn){}

BoundedMergingSearcher::~BoundedMergingSearcher() {
  delete baseSearcher;
}

ExecutionState& BoundedMergingSearcher::selectState() {
  assert(!baseSearcher->empty() && "base searcher is empty");

  if (!IncompleteBoundedMerge) {
    return baseSearcher->selectState();
  }
  // Iterate through all BoundedMergeHandlers
  for (std::vector<BoundedMergeHandler *>::iterator
           bmh_it = executor.mergeGroups.begin(),
           bmh_ie = executor.mergeGroups.end();
       bmh_it != bmh_ie; ++bmh_it) {
    // Find one that has states that could be released
    if (!(*bmh_it)->hasMergedStates()) {
      continue;
    }
    // Find a state that can be prioritized
    ExecutionState *es = (*bmh_it)->getPrioritizeState();
    if (es) {
      return *es;
    } else {
      if (DebugLogIncompleteMerge){
        llvm::errs() << "Preemptively releasing states\n";
      }
      // If no state can be prioritized, they all exceeded the amount of time we
      // are willing to wait for them. Release the states that already made it.
      (*bmh_it)->releaseStates();
    }
  }
  // If we were not able to prioritize a merging state, just return some state
  return baseSearcher->selectState();
}

void
BoundedMergingSearcher::update(ExecutionState *current,
    const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  baseSearcher->update(current, addedStates, removedStates);
}

///

BatchingSearcher::BatchingSearcher(Searcher *_baseSearcher,
                                   double _timeBudget,
                                   unsigned _instructionBudget) 
  : baseSearcher(_baseSearcher),
    timeBudget(_timeBudget),
    instructionBudget(_instructionBudget),
    lastState(0) {
  
}

BatchingSearcher::~BatchingSearcher() {
  delete baseSearcher;
}

ExecutionState &BatchingSearcher::selectState() {
  if (!lastState || 
      (util::getWallTime()-lastStartTime)>timeBudget ||
      (stats::instructions-lastStartInstructions)>instructionBudget) {
    if (lastState) {
      double delta = util::getWallTime()-lastStartTime;
      if (delta>timeBudget*1.1) {
        klee_message("increased time budget from %f to %f\n", timeBudget,
                     delta);
        timeBudget = delta;
      }
    }
    lastState = &baseSearcher->selectState();
    lastStartTime = util::getWallTime();
    lastStartInstructions = stats::instructions;
    return *lastState;
  } else {
    return *lastState;
  }
}

void
BatchingSearcher::update(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates) {
  if (std::find(removedStates.begin(), removedStates.end(), lastState) !=
      removedStates.end())
    lastState = 0;
  baseSearcher->update(current, addedStates, removedStates);
}

/***/

IterativeDeepeningTimeSearcher::IterativeDeepeningTimeSearcher(Searcher *_baseSearcher)
  : baseSearcher(_baseSearcher),
    time(1.) {
}

IterativeDeepeningTimeSearcher::~IterativeDeepeningTimeSearcher() {
  delete baseSearcher;
}

ExecutionState &IterativeDeepeningTimeSearcher::selectState() {
  ExecutionState &res = baseSearcher->selectState();
  startTime = util::getWallTime();
  return res;
}

void IterativeDeepeningTimeSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  double elapsed = util::getWallTime() - startTime;

  if (!removedStates.empty()) {
    std::vector<ExecutionState *> alt = removedStates;
    for (std::vector<ExecutionState *>::const_iterator
             it = removedStates.begin(),
             ie = removedStates.end();
         it != ie; ++it) {
      ExecutionState *es = *it;
      std::set<ExecutionState*>::const_iterator it2 = pausedStates.find(es);
      if (it2 != pausedStates.end()) {
        pausedStates.erase(it2);
        alt.erase(std::remove(alt.begin(), alt.end(), es), alt.end());
      }
    }    
    baseSearcher->update(current, addedStates, alt);
  } else {
    baseSearcher->update(current, addedStates, removedStates);
  }

  if (current &&
      std::find(removedStates.begin(), removedStates.end(), current) ==
          removedStates.end() &&
      elapsed > time) {
    pausedStates.insert(current);
    baseSearcher->removeState(current);
  }

  if (baseSearcher->empty()) {
    time *= 2;
    klee_message("increased time budget to %f\n", time);
    std::vector<ExecutionState *> ps(pausedStates.begin(), pausedStates.end());
    baseSearcher->update(0, ps, std::vector<ExecutionState *>());
    pausedStates.clear();
  }
}

/***/

InterleavedSearcher::InterleavedSearcher(const std::vector<Searcher*> &_searchers)
  : searchers(_searchers),
    index(1) {
}

InterleavedSearcher::~InterleavedSearcher() {
  for (std::vector<Searcher*>::const_iterator it = searchers.begin(),
         ie = searchers.end(); it != ie; ++it)
    delete *it;
}

ExecutionState &InterleavedSearcher::selectState() {
  Searcher *s = searchers[--index];
  if (index==0) index = searchers.size();
  return s->selectState();
}

void InterleavedSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  for (std::vector<Searcher*>::const_iterator it = searchers.begin(),
         ie = searchers.end(); it != ie; ++it)
    (*it)->update(current, addedStates, removedStates);
}
