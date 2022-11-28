//===-- Target.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Target.h"
#include "ExecutionState.h"

#include "klee/Module/CodeGraphDistance.h"
#include "klee/Module/KInstruction.h"

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

std::string Target::toString() const {
  std::string repr = "Target: ";
  repr += block->getAssemblyLocation();
  if (atReturn()) {
    repr += " (at the end)";
  }
  return repr;
}

void TargetCalculator::update(const ExecutionState &state) {
  switch (TargetCalculatorMode) {
  case TargetCalculateBy::Default:
    blocksHistory[state.getInitPCBlock()][state.getPrevPCBlock()].insert(
        state.getInitPCBlock());
    break;

  case TargetCalculateBy::Blocks:
    blocksHistory[state.getInitPCBlock()][state.getPrevPCBlock()].insert(
        state.level.begin(), state.level.end());
    break;

  case TargetCalculateBy::Transitions:
    blocksHistory[state.getInitPCBlock()][state.getPrevPCBlock()].insert(
        state.level.begin(), state.level.end());
    transitionsHistory[state.getInitPCBlock()][state.getPrevPCBlock()].insert(
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

Target TargetCalculator::calculate(ExecutionState &state) {
  BasicBlock *initialBlock = state.getInitPCBlock();
  std::unordered_map<llvm::BasicBlock *, VisitedBlocks> &history =
      blocksHistory[initialBlock];
  std::unordered_map<llvm::BasicBlock *, VisitedTransitions>
      &transitionHistory = transitionsHistory[initialBlock];
  BasicBlock *bb = state.getPCBlock();
  KFunction *kf = module.functionMap.at(bb->getParent());
  KBlock *kb = kf->blockMap[bb];
  KBlock *nearestBlock = nullptr;
  unsigned int minDistance = UINT_MAX;
  unsigned int sfNum = 0;
  bool newCov = false;
  for (auto sfi = state.stack.rbegin(), sfe = state.stack.rend(); sfi != sfe;
       sfi++, sfNum++) {
    kf = sfi->kf;

    for (const auto &kbd : codeGraphDistance.getSortedDistance(kb)) {
      KBlock *target = kbd.first;
      unsigned distance = kbd.second;
      if ((sfNum > 0 || distance > 0)) {
        if (distance >= minDistance)
          break;
        if (history[target->basicBlock].size() != 0) {
          bool diffIsEmpty = true;
          if (!newCov) {
            switch (TargetCalculatorMode) {
            case TargetCalculateBy::Blocks:
              diffIsEmpty = differenceIsEmpty(state, history, target);
              break;
            case TargetCalculateBy::Transitions:
              diffIsEmpty = differenceIsEmpty(state, transitionHistory, target);
              break;
            case TargetCalculateBy::Default:
              break;
            }
          }

          if (diffIsEmpty) {
            continue;
          }
        } else {
          newCov = true;
        }
        nearestBlock = target;
        minDistance = distance;
      }
    }

    if (nearestBlock) {
      return Target(nearestBlock);
    }

    if (sfi->caller) {
      kb = sfi->caller->parent;
    }
  }

  return Target(nearestBlock);
}
