//===-- CodeGraphInfo.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/CodeGraphInfo.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/CFG.h"
DISABLE_WARNING_POP

#include <deque>
#include <unordered_map>

using namespace klee;

void CodeGraphInfo::calculateDistance(Block *bb) {
  auto &dist = blockDistance[bb];
  auto &sort = blockSortedDistance[bb];
  std::deque<Block *> nodes;
  nodes.push_back(bb);
  dist[bb] = 0;
  sort.push_back({bb, 0});
  bool hasCycle = false;
  for (; !nodes.empty(); nodes.pop_front()) {
    auto currBB = nodes.front();
    for (auto succ : successors(currBB)) {
      if (succ == bb) {
        hasCycle = true;
        continue;
      }
      if (dist.count(succ))
        continue;
      auto d = dist[currBB] + 1;
      dist.emplace(succ, d);
      sort.push_back({succ, d});
      nodes.push_back(succ);
    }
  }
  if (hasCycle)
    blockCycles.insert(bb);
}

void CodeGraphInfo::calculateBackwardDistance(Block *bb) {
  auto &bdist = blockBackwardDistance[bb];
  auto &bsort = blockSortedBackwardDistance[bb];
  std::deque<Block *> nodes;
  nodes.push_back(bb);
  bdist[bb] = 0;
  bsort.push_back({bb, 0});
  while (!nodes.empty()) {
    auto currBB = nodes.front();
    for (auto const &pred : predecessors(currBB)) {
      if (bdist.count(pred) == 0) {
        bdist[pred] = bdist[currBB] + 1;
        bsort.push_back({pred, bdist[currBB] + 1});
        nodes.push_back(pred);
      }
    }
    nodes.pop_front();
  }
}

void CodeGraphInfo::calculateDistance(KFunction *kf) {
  auto f = kf->function;
  auto &functionMap = kf->parent->functionMap;
  auto &dist = functionDistance[f];
  auto &sort = functionSortedDistance[f];
  std::deque<KFunction *> nodes;
  nodes.push_back(kf);
  dist[f] = 0;
  sort.push_back({f, 0});
  while (!nodes.empty()) {
    auto currKF = nodes.front();
    for (auto callBlock : currKF->kCallBlocks) {
      for (auto calledFunction : callBlock->calledFunctions) {
        if (!calledFunction || calledFunction->isDeclaration())
          continue;
        if (dist.count(calledFunction) == 0) {
          auto d = dist[currKF->function] + 1;
          dist[calledFunction] = d;
          sort.emplace_back(calledFunction, d);
          auto callKF = functionMap[calledFunction];
          nodes.push_back(callKF);
        }
      }
    }
    nodes.pop_front();
  }
}

void CodeGraphInfo::calculateBackwardDistance(KFunction *kf) {
  auto f = kf->function;
  auto &callMap = kf->parent->callMap;
  auto &bdist = functionBackwardDistance[f];
  auto &bsort = functionSortedBackwardDistance[f];
  std::deque<llvm::Function *> nodes = {f};
  bdist[f] = 0;
  bsort.emplace_back(f, 0);
  for (; !nodes.empty(); nodes.pop_front()) {
    auto currKF = nodes.front();
    for (auto cf : callMap[currKF]) {
      if (cf->isDeclaration())
        continue;
      auto it = bdist.find(cf);
      if (it == bdist.end()) {
        auto d = bdist[currKF] + 1;
        bdist.emplace_hint(it, cf, d);
        bsort.emplace_back(cf, d);
        nodes.push_back(cf);
      }
    }
  }
}

void CodeGraphInfo::calculateFunctionBranches(KFunction *kf) {
  std::map<KBlock *, std::set<unsigned>> &fbranches = functionBranches[kf];
  for (auto &kb : kf->blocks) {
    fbranches[kb.get()];
    for (unsigned branch = 0;
         branch < kb->basicBlock->getTerminator()->getNumSuccessors();
         ++branch) {
      fbranches[kb.get()].insert(branch);
    }
  }
}
void CodeGraphInfo::calculateFunctionConditionalBranches(KFunction *kf) {
  std::map<KBlock *, std::set<unsigned>> &fbranches =
      functionConditionalBranches[kf];
  for (auto &kb : kf->blocks) {
    if (kb->basicBlock->getTerminator()->getNumSuccessors() > 1) {
      fbranches[kb.get()];
      for (unsigned branch = 0;
           branch < kb->basicBlock->getTerminator()->getNumSuccessors();
           ++branch) {
        fbranches[kb.get()].insert(branch);
      }
    }
  }
}
void CodeGraphInfo::calculateFunctionBlocks(KFunction *kf) {
  std::map<KBlock *, std::set<unsigned>> &fbranches = functionBlocks[kf];
  for (auto &kb : kf->blocks) {
    fbranches[kb.get()];
  }
}

const BlockDistanceMap &CodeGraphInfo::getDistance(Block *b) {
  if (blockDistance.count(b) == 0)
    calculateDistance(b);
  return blockDistance.at(b);
}

bool CodeGraphInfo::hasCycle(KBlock *kb) {
  auto b = kb->basicBlock;
  if (!blockDistance.count(b))
    calculateDistance(b);
  return blockCycles.count(b);
}

const BlockDistanceMap &CodeGraphInfo::getDistance(KBlock *kb) {
  return getDistance(kb->basicBlock);
}

const BlockDistanceMap &CodeGraphInfo::getBackwardDistance(KBlock *kb) {
  if (blockBackwardDistance.count(kb->basicBlock) == 0)
    calculateBackwardDistance(kb->basicBlock);
  return blockBackwardDistance.at(kb->basicBlock);
}

const FunctionDistanceMap &CodeGraphInfo::getDistance(KFunction *kf) {
  auto f = kf->function;
  if (functionDistance.count(f) == 0)
    calculateDistance(kf);
  return functionDistance.at(f);
}

const FunctionDistanceMap &CodeGraphInfo::getBackwardDistance(KFunction *kf) {
  auto f = kf->function;
  if (functionBackwardDistance.count(f) == 0)
    calculateBackwardDistance(kf);
  return functionBackwardDistance.at(f);
}

void CodeGraphInfo::getNearestPredicateSatisfying(KBlock *from,
                                                  KBlockPredicate predicate,
                                                  std::set<KBlock *> &result) {
  std::set<KBlock *> visited;

  auto blockMap = from->parent->blockMap;
  std::deque<KBlock *> nodes;
  nodes.push_back(from);

  while (!nodes.empty()) {
    KBlock *currBB = nodes.front();
    visited.insert(currBB);

    if (predicate(currBB) && currBB != from) {
      result.insert(currBB);
    } else {
      for (auto succ : successors(currBB->basicBlock)) {
        if (visited.count(blockMap[succ]) == 0) {
          nodes.push_back(blockMap[succ]);
        }
      }
    }
    nodes.pop_front();
  }
}

const std::map<KBlock *, std::set<unsigned>> &
CodeGraphInfo::getFunctionBranches(KFunction *kf) {
  if (functionBranches.count(kf) == 0)
    calculateFunctionBranches(kf);
  return functionBranches.at(kf);
}

const std::map<KBlock *, std::set<unsigned>> &
CodeGraphInfo::getFunctionConditionalBranches(KFunction *kf) {
  if (functionConditionalBranches.count(kf) == 0)
    calculateFunctionConditionalBranches(kf);
  return functionConditionalBranches.at(kf);
}

const std::map<KBlock *, std::set<unsigned>> &
CodeGraphInfo::getFunctionBlocks(KFunction *kf) {
  if (functionBlocks.count(kf) == 0)
    calculateFunctionBlocks(kf);
  return functionBlocks.at(kf);
}
