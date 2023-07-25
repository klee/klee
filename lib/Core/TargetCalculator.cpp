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

#include "klee/Module/CodeGraphDistance.h"
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
  switch (TargetCalculatorMode) {
  case TargetCalculateBy::Default:
    blocksHistory[initialFunction][state.getPrevPCBlock()].insert(
        state.getInitPCBlock());
    if (state.prevPC == state.prevPC->parent->getLastInstruction()) {
      coveredBlocks[state.getPrevPCBlock()->getParent()].insert(
          state.getPrevPCBlock());
    }
    if (state.prevPC == state.prevPC->parent->getLastInstruction()) {
      unsigned index = 0;
      coveredBranches[state.getPrevPCBlock()->getParent()]
                     [state.getPrevPCBlock()];
      for (auto succ : successors(state.getPrevPCBlock())) {
        if (succ == state.getPCBlock()) {
          coveredBranches[state.getPrevPCBlock()->getParent()]
                         [state.getPrevPCBlock()]
                             .insert(index);
          break;
        }
        ++index;
      }
    }
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

bool TargetCalculator::differenceIsEmpty(
    const ExecutionState &state,
    const std::unordered_map<llvm::BasicBlock *, VisitedBlocks> &history,
    KBlock *target) {
  std::vector<BasicBlock *> diff;
  std::set<BasicBlock *> left(state.level.begin(), state.level.end());
  std::set<BasicBlock *> right(history.at(target->basicBlock).begin(),
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
  switch (TargetCalculatorMode) {
  case TargetCalculateBy::Default: {
    if (coveredBranches[kblock->parent->function].count(kblock->basicBlock) ==
        0) {
      result = true;
    } else {
      auto &cb = coveredBranches[kblock->parent->function][kblock->basicBlock];
      result =
          kblock->basicBlock->getTerminator()->getNumSuccessors() > cb.size();
    }
    break;
  }
  case TargetCalculateBy::Blocks: {
    if (history[kblock->basicBlock].size() != 0) {
      result = !differenceIsEmpty(*state, history, kblock);
    }
    break;
  }
  case TargetCalculateBy::Transitions: {
    if (history[kblock->basicBlock].size() != 0) {
      result = !differenceIsEmpty(*state, transitionHistory, kblock);
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
  for (auto sfi = state.stack.callStack().rbegin(),
            sfe = state.stack.callStack().rend();
       sfi != sfe; sfi++) {
    kf = sfi->kf;

    std::set<KBlock *> blocks;
    using std::placeholders::_1;
    KBlockPredicate func =
        std::bind(&TargetCalculator::uncoveredBlockPredicate, this, &state, _1);
    codeGraphDistance.getNearestPredicateSatisfying(kb, func, blocks);

    if (!blocks.empty()) {
      TargetHashSet targets;
      for (auto block : blocks) {
        if (coveredBranches[block->parent->function].count(block->basicBlock) ==
            0) {
          targets.insert(ReachBlockTarget::create(block, true));
        } else {
          auto &cb =
              coveredBranches[block->parent->function][block->basicBlock];
          for (unsigned index = 0;
               index < block->basicBlock->getTerminator()->getNumSuccessors();
               ++index) {
            if (!cb.count(index))
              targets.insert(CoverBranchTarget::create(block, index));
          }
        }
      }
      return targets;
    }

    if (sfi->caller) {
      kb = sfi->caller->parent;
    }
  }

  return {};
}
