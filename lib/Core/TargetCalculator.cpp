//===-- TargetCalculator.cpp ---------- -----------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TargetCalculator.h"

#include "ExecutionState.h"

#include "klee/Module/CodeGraphInfo.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/Target.h"
#include "klee/Module/TargetHash.h"

#include <set>
#include <vector>

using namespace llvm;
using namespace klee;

namespace klee {
llvm::cl::opt<TargetCalculateBy> TargetCalculatorMode(
    "target-calculator-kind", cl::desc("Specifiy the target calculator mode."),
    cl::values(
        clEnumValN(TargetCalculateBy::Default, "default",
                   "Looks for the closest uncovered block."),
        clEnumValN(
            TargetCalculateBy::Blocks, "blocks",
            "Looks for the closest uncovered block by state blocks history."),
        clEnumValN(TargetCalculateBy::Transitions, "transitions",
                   "Looks for the closest uncovered block by state transitions "
                   "history.")),
    cl::init(TargetCalculateBy::Default), cl::cat(ExecCat));
} // namespace klee

void TargetCalculator::update(const ExecutionState &state) {
  Function *initialFunction = state.getInitPCBlock()->getParent();

  if (state.prevPC == state.prevPC->parent->getLastInstruction()) {
    coveredBlocks[state.getPrevPCBlock()->getParent()].insert(
        state.prevPC->parent);
  }
  if (state.prevPC == state.prevPC->parent->getLastInstruction() &&
      !fullyCoveredFunctions.count(state.prevPC->parent->parent)) {

    if (!coveredFunctionsInBranches.count(state.prevPC->parent->parent)) {
      unsigned index = 0;
      if (!coveredBranches[state.prevPC->parent->parent].count(
              state.prevPC->parent)) {
        state.coverNew();
        coveredBranches[state.prevPC->parent->parent][state.prevPC->parent];
      }
      for (auto succ : successors(state.getPrevPCBlock())) {
        if (succ == state.getPCBlock()) {
          if (!coveredBranches[state.prevPC->parent->parent]
                              [state.prevPC->parent]
                                  .count(index)) {
            state.coverNew();
            coveredBranches[state.prevPC->parent->parent][state.prevPC->parent]
                .insert(index);
          }
          break;
        }
        ++index;
      }
      if (codeGraphInfo.getFunctionBranches(state.prevPC->parent->parent) ==
          coveredBranches[state.prevPC->parent->parent]) {
        coveredFunctionsInBranches.insert(state.prevPC->parent->parent);
      }
    }
    if (!fullyCoveredFunctions.count(state.prevPC->parent->parent) &&
        coveredFunctionsInBranches.count(state.prevPC->parent->parent)) {
      bool covered = true;
      std::set<KFunction *> fnsTaken;
      std::deque<KFunction *> fns;
      fns.push_back(state.prevPC->parent->parent);

      while (!fns.empty() && covered) {
        KFunction *currKF = fns.front();
        fnsTaken.insert(currKF);
        for (auto &kcallBlock : currKF->kCallBlocks) {
          if (kcallBlock->calledFunctions.size() == 1) {
            auto calledFunction = *kcallBlock->calledFunctions.begin();
            KFunction *calledKFunction = state.prevPC->parent->parent->parent
                                             ->functionMap[calledFunction];
            if (calledKFunction->numInstructions != 0 &&
                coveredFunctionsInBranches.count(calledKFunction) == 0) {
              covered = false;
              break;
            }
            if (!fnsTaken.count(calledKFunction) &&
                fullyCoveredFunctions.count(calledKFunction) == 0) {
              fns.push_back(calledKFunction);
            }
          }
        }
        fns.pop_front();
      }

      if (covered) {
        fullyCoveredFunctions.insert(state.prevPC->parent->parent);
      }
    }
  }

  switch (TargetCalculatorMode) {
  case TargetCalculateBy::Default:
    blocksHistory[initialFunction][state.getPrevPCBlock()].insert(
        state.initPC->parent);
    break;

  case TargetCalculateBy::Blocks:
    blocksHistory[initialFunction][state.getPrevPCBlock()].insert(
        state.level.begin(), state.level.end());
    break;

  case TargetCalculateBy::Transitions:
    blocksHistory[initialFunction][state.getPrevPCBlock()].insert(
        state.level.begin(), state.level.end());
    transitionsHistory[initialFunction][state.getPrevPCBlock()].insert(
        state.transitionLevel.begin(), state.transitionLevel.end());
    break;
  }
}

void TargetCalculator::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  if (current && (std::find(removedStates.begin(), removedStates.end(),
                            current) == removedStates.end())) {
    localStates.insert(current);
  }
  for (const auto state : addedStates) {
    localStates.insert(state);
  }
  for (const auto state : removedStates) {
    localStates.insert(state);
  }
  for (auto state : localStates) {
    KFunction *kf = state->prevPC->parent->parent;
    KModule *km = kf->parent;
    if (state->prevPC->inst->isTerminator() &&
        km->inMainModule(*kf->function)) {
      update(*state);
    }
  }
  localStates.clear();
}

bool TargetCalculator::differenceIsEmpty(
    const ExecutionState &state,
    const std::unordered_map<llvm::BasicBlock *, VisitedBlocks> &history,
    KBlock *target) {
  std::vector<KBlock *> diff;
  std::set<KBlock *> left(state.level.begin(), state.level.end());
  std::set<KBlock *> right(history.at(target->basicBlock).begin(),
                           history.at(target->basicBlock).end());
  std::set_difference(left.begin(), left.end(), right.begin(), right.end(),
                      std::inserter(diff, diff.begin()));
  return diff.empty();
}

bool TargetCalculator::differenceIsEmpty(
    const ExecutionState &state,
    const std::unordered_map<llvm::BasicBlock *, VisitedTransitions> &history,
    KBlock *target) {
  std::vector<Transition> diff;
  std::set<Transition> left(state.transitionLevel.begin(),
                            state.transitionLevel.end());
  std::set<Transition> right(history.at(target->basicBlock).begin(),
                             history.at(target->basicBlock).end());
  std::set_difference(left.begin(), left.end(), right.begin(), right.end(),
                      std::inserter(diff, diff.begin()));
  return diff.empty();
}

bool TargetCalculator::uncoveredBlockPredicate(ExecutionState *state,
                                               KBlock *kblock) {
  Function *initialFunction = state->getInitPCBlock()->getParent();
  std::unordered_map<llvm::BasicBlock *, VisitedBlocks> &history =
      blocksHistory[initialFunction];
  std::unordered_map<llvm::BasicBlock *, VisitedTransitions>
      &transitionHistory = transitionsHistory[initialFunction];
  bool result = false;
  if (coveredBranches[kblock->parent].count(kblock) == 0) {
    result = true;
  } else {
    auto &cb = coveredBranches[kblock->parent][kblock];
    if (isa<KCallBlock>(kblock) &&
        cast<KCallBlock>(kblock)->calledFunctions.size() == 1) {
      auto calledFunction = *cast<KCallBlock>(kblock)->calledFunctions.begin();
      KFunction *calledKFunction =
          kblock->parent->parent->functionMap[calledFunction];
      result = fullyCoveredFunctions.count(calledKFunction) == 0 &&
               calledKFunction->numInstructions;
    }
    result |=
        kblock->basicBlock->getTerminator()->getNumSuccessors() > cb.size();
  }

  switch (TargetCalculatorMode) {
  case TargetCalculateBy::Default: {
    break;
  }
  case TargetCalculateBy::Blocks: {
    if (history[kblock->basicBlock].size() != 0) {
      result |= !differenceIsEmpty(*state, history, kblock);
    }
    break;
  }
  case TargetCalculateBy::Transitions: {
    if (history[kblock->basicBlock].size() != 0) {
      result |= !differenceIsEmpty(*state, transitionHistory, kblock);
    }
    break;
  }
  }

  return result;
}

TargetHashSet TargetCalculator::calculate(ExecutionState &state) {
  BasicBlock *bb = state.getPCBlock();
  const KModule &module = *state.pc->parent->parent->parent;
  KFunction *kf = module.functionMap.at(bb->getParent());
  KBlock *kb = kf->blockMap[bb];
  kb = !isa<KCallBlock>(kb) || (kb->getLastInstruction() != state.pc)
           ? kb
           : kf->blockMap[state.pc->parent->basicBlock->getTerminator()
                              ->getSuccessor(0)];
  for (auto sfi = state.stack.callStack().rbegin(),
            sfe = state.stack.callStack().rend();
       sfi != sfe; sfi++) {
    kf = sfi->kf;

    std::set<KBlock *> blocks;
    using std::placeholders::_1;
    KBlockPredicate func =
        std::bind(&TargetCalculator::uncoveredBlockPredicate, this, &state, _1);
    codeGraphInfo.getNearestPredicateSatisfying(kb, func, blocks);

    if (!blocks.empty()) {
      TargetHashSet targets;
      for (auto block : blocks) {
        if (coveredBranches[block->parent].count(block) == 0) {
          targets.insert(ReachBlockTarget::create(block, false));
        } else {
          auto &cb = coveredBranches[block->parent][block];
          bool notCoveredFunction = false;
          if (isa<KCallBlock>(block) &&
              cast<KCallBlock>(block)->calledFunctions.size() == 1) {
            auto calledFunction =
                *cast<KCallBlock>(block)->calledFunctions.begin();
            KFunction *calledKFunction =
                block->parent->parent->functionMap[calledFunction];
            notCoveredFunction =
                fullyCoveredFunctions.count(calledKFunction) == 0 &&
                calledKFunction->numInstructions;
          }
          if (notCoveredFunction) {
            targets.insert(ReachBlockTarget::create(block, true));
          } else {
            for (unsigned index = 0;
                 index < block->basicBlock->getTerminator()->getNumSuccessors();
                 ++index) {
              if (!cb.count(index))
                targets.insert(CoverBranchTarget::create(block, index));
            }
          }
        }
      }
      assert(!targets.empty());
      return targets;
    }

    if (sfi->caller) {
      kb = sfi->caller->parent;

      kb = !isa<KCallBlock>(kb) || (kb->getLastInstruction() != sfi->caller)
               ? kb
               : kf->blockMap[sfi->caller->parent->basicBlock->getTerminator()
                                  ->getSuccessor(0)];
    }
  }

  return {};
}
