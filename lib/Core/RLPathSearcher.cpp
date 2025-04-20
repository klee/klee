//===-- RLPathSearcher.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RLPathSearcher.h"
#include "ExecutionState.h"
#include "ExecutionTree.h"
#include "klee/Statistics/Statistics.h"
#include "klee/Module/KInstruction.h"

namespace klee {

RLPathSearcher::RLPathSearcher(InMemoryExecutionTree *executionTree, RNG &rng)
    : executionTree{executionTree}, theRNG{rng},
      idBitMask{static_cast<uint8_t>(executionTree ? executionTree->getNextId() : 0)} {
  assert(executionTree);
}

float RLPathSearcher::calculateReward(ExecutionTreeNode* node) {
  if (!node || !node->state) {
    return 0.0f;
  }
  
  float reward = 0.0f;
  
  // Reward for new coverage
  if (node->state->coveredNew) {
    reward += 10.0f;
  }
  
  // Reward for finding errors
  if (node->state->hasError()) {
    reward += 100.0f;
  }
  
  // Penalty for depth to encourage exploration
  reward -= 0.01f * node->depth;
  
  // Penalty for high query cost
  reward -= 0.1f * node->state->queryMetaData.queryCost.toSeconds();
  
  return reward;
}

ExecutionState &RLPathSearcher::selectState() {
  unsigned flips=0, bits=0;
  assert(executionTree->root.getInt() & idBitMask &&
         "Root should belong to the searcher");
  
  ExecutionTreeNode *n = executionTree->root.getPointer();
  ExecutionTreeNode *lastNode = n;
  
  // Walk down the tree using RL to guide decisions
  while (!n->state) {
    lastNode = n;
    
    if (!IS_OUR_NODE_VALID(n->left)) {
      assert(IS_OUR_NODE_VALID(n->right) && "Both left and right nodes invalid");
      assert(n != n->right.getPointer());
      n = n->right.getPointer();
    } else if (!IS_OUR_NODE_VALID(n->right)) {
      assert(IS_OUR_NODE_VALID(n->left) && "Both right and left nodes invalid");
      assert(n != n->left.getPointer());
      n = n->left.getPointer();
    } else {
      // Use RL to choose between left and right paths
      bool goLeft = rlInterface.chooseAction(n->left.getPointer(), n->right.getPointer());
      n = (goLeft ? n->left : n->right).getPointer();
    }
  }
  
  // Calculate reward for the transition
  float reward = calculateReward(n);
  
  // Add experience to the RL model
  if (lastVisitedNode) {
    rlInterface.addExperience(
      lastVisitedNode,                  // state
      lastVisitedNode->left.getPointer() == n, // action (true if we went left)
      reward,                           // reward
      n,                                // next state
      n->state->isTerminated()          // done
    );
  }
  
  // Update last visited node
  lastVisitedNode = n;
  
  return *n->state;
}

void RLPathSearcher::update(ExecutionState *current,
                            const std::vector<ExecutionState *> &addedStates,
                            const std::vector<ExecutionState *> &removedStates) {
  // Similar to RandomPathSearcher, mark nodes as ours
  
  // insert states
  for (auto es : addedStates) {
    ExecutionTreeNode *etnode = es->executionTreeNode, *parent = etnode->parent;
    ExecutionTreeNodePtr *childPtr;

    childPtr = parent ? ((parent->left.getPointer() == etnode) ? &parent->left
                                                               : &parent->right)
                      : &executionTree->root;
    while (etnode && !IS_OUR_NODE_VALID(*childPtr)) {
      childPtr->setInt(childPtr->getInt() | idBitMask);
      etnode = parent;
      if (etnode)
        parent = etnode->parent;

      childPtr = parent
                     ? ((parent->left.getPointer() == etnode) ? &parent->left
                                                              : &parent->right)
                     : &executionTree->root;
    }
  }

  // remove states
  for (auto es : removedStates) {
    ExecutionTreeNode *etnode = es->executionTreeNode, *parent = etnode->parent;

    while (etnode && !IS_OUR_NODE_VALID(etnode->left) &&
           !IS_OUR_NODE_VALID(etnode->right)) {
      auto childPtr =
          parent ? ((parent->left.getPointer() == etnode) ? &parent->left
                                                          : &parent->right)
                 : &executionTree->root;
      assert(IS_OUR_NODE_VALID(*childPtr) &&
             "Removing executionTree child not ours");
      childPtr->setInt(childPtr->getInt() & ~idBitMask);
      etnode = parent;
      if (etnode)
        parent = etnode->parent;
    }
  }
}

bool RLPathSearcher::empty() {
  return !IS_OUR_NODE_VALID(executionTree->root);
}

void RLPathSearcher::printName(llvm::raw_ostream &os) {
  os << "RLPathSearcher\n";
}

} // namespace klee
