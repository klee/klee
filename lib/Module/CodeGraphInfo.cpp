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

void CodeGraphInfo::calculateDistance(KBlock *bb) {
  auto blockMap = bb->parent->blockMap;
  std::unordered_map<KBlock *, unsigned int> &dist = blockDistance[bb];
  std::vector<std::pair<KBlock *, unsigned>> &sort = blockSortedDistance[bb];
  std::deque<KBlock *> nodes;
  nodes.push_back(bb);
  dist[bb] = 0;
  sort.push_back({bb, 0});
  while (!nodes.empty()) {
    KBlock *currBB = nodes.front();
    for (auto const &succ : successors(currBB->basicBlock)) {
      if (dist.count(blockMap[succ]) == 0) {
        dist[blockMap[succ]] = dist[currBB] + 1;
        sort.push_back({blockMap[succ], dist[currBB] + 1});
        nodes.push_back(blockMap[succ]);
      }
    }
    nodes.pop_front();
  }
}

void CodeGraphInfo::calculateBackwardDistance(KBlock *bb) {
  auto blockMap = bb->parent->blockMap;
  std::unordered_map<KBlock *, unsigned int> &bdist = blockBackwardDistance[bb];
  std::vector<std::pair<KBlock *, unsigned>> &bsort =
      blockSortedBackwardDistance[bb];
  std::deque<KBlock *> nodes;
  nodes.push_back(bb);
  bdist[bb] = 0;
  bsort.push_back({bb, 0});
  while (!nodes.empty()) {
    KBlock *currBB = nodes.front();
    for (auto const &pred : predecessors(currBB->basicBlock)) {
      if (bdist.count(blockMap[pred]) == 0) {
        bdist[blockMap[pred]] = bdist[currBB] + 1;
        bsort.push_back({blockMap[pred], bdist[currBB] + 1});
        nodes.push_back(blockMap[pred]);
      }
    }
    nodes.pop_front();
  }
}

void CodeGraphInfo::calculateDistance(KFunction *kf) {
  auto &functionMap = kf->parent->functionMap;
  std::unordered_map<KFunction *, unsigned int> &dist = functionDistance[kf];
  std::vector<std::pair<KFunction *, unsigned>> &sort =
      functionSortedDistance[kf];
  std::deque<KFunction *> nodes;
  nodes.push_back(kf);
  dist[kf] = 0;
  sort.push_back({kf, 0});
  while (!nodes.empty()) {
    KFunction *currKF = nodes.front();
    for (auto &callBlock : currKF->kCallBlocks) {
      for (auto &calledFunction : callBlock->calledFunctions) {
        if (!calledFunction || calledFunction->isDeclaration()) {
          continue;
        }
        KFunction *callKF = functionMap[calledFunction];
        if (dist.count(callKF) == 0) {
          dist[callKF] = dist[currKF] + 1;
          sort.push_back({callKF, dist[currKF] + 1});
          nodes.push_back(callKF);
        }
      }
    }
    nodes.pop_front();
  }
}

void CodeGraphInfo::calculateBackwardDistance(KFunction *kf) {
  auto &functionMap = kf->parent->functionMap;
  auto &callMap = kf->parent->callMap;
  std::unordered_map<KFunction *, unsigned int> &bdist =
      functionBackwardDistance[kf];
  std::vector<std::pair<KFunction *, unsigned>> &bsort =
      functionSortedBackwardDistance[kf];
  std::deque<KFunction *> nodes;
  nodes.push_back(kf);
  bdist[kf] = 0;
  bsort.push_back({kf, 0});
  while (!nodes.empty()) {
    KFunction *currKF = nodes.front();
    for (auto &cf : callMap[currKF->function]) {
      if (cf->isDeclaration()) {
        continue;
      }
      KFunction *callKF = functionMap[cf];
      if (bdist.count(callKF) == 0) {
        bdist[callKF] = bdist[currKF] + 1;
        bsort.push_back({callKF, bdist[currKF] + 1});
        nodes.push_back(callKF);
      }
    }
    nodes.pop_front();
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

const std::unordered_map<KBlock *, unsigned> &
CodeGraphInfo::getDistance(KBlock *kb) {
  if (blockDistance.count(kb) == 0)
    calculateDistance(kb);
  return blockDistance.at(kb);
}

const std::unordered_map<KBlock *, unsigned> &
CodeGraphInfo::getBackwardDistance(KBlock *kb) {
  if (blockBackwardDistance.count(kb) == 0)
    calculateBackwardDistance(kb);
  return blockBackwardDistance.at(kb);
}

const std::vector<std::pair<KBlock *, unsigned int>> &
CodeGraphInfo::getSortedDistance(KBlock *kb) {
  if (blockDistance.count(kb) == 0)
    calculateDistance(kb);
  return blockSortedDistance.at(kb);
}

const std::vector<std::pair<KBlock *, unsigned int>> &
CodeGraphInfo::getSortedBackwardDistance(KBlock *kb) {
  if (blockBackwardDistance.count(kb) == 0)
    calculateBackwardDistance(kb);
  return blockSortedBackwardDistance.at(kb);
}

const std::unordered_map<KFunction *, unsigned> &
CodeGraphInfo::getDistance(KFunction *kf) {
  if (functionDistance.count(kf) == 0)
    calculateDistance(kf);
  return functionDistance.at(kf);
}

const std::unordered_map<KFunction *, unsigned> &
CodeGraphInfo::getBackwardDistance(KFunction *kf) {
  if (functionBackwardDistance.count(kf) == 0)
    calculateBackwardDistance(kf);
  return functionBackwardDistance.at(kf);
}

const std::vector<std::pair<KFunction *, unsigned int>> &
CodeGraphInfo::getSortedDistance(KFunction *kf) {
  if (functionDistance.count(kf) == 0)
    calculateDistance(kf);
  return functionSortedDistance.at(kf);
}

const std::vector<std::pair<KFunction *, unsigned int>> &
CodeGraphInfo::getSortedBackwardDistance(KFunction *kf) {
  if (functionBackwardDistance.count(kf) == 0)
    calculateBackwardDistance(kf);
  return functionSortedBackwardDistance.at(kf);
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
      for (auto const &succ : successors(currBB->basicBlock)) {
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
