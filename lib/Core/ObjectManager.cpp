#include "ObjectManager.h"

#include "CoreStats.h"
#include "PForest.h"
#include "TargetManager.h"

#include "klee/Module/KModule.h"

using namespace llvm;
using namespace klee;

ObjectManager::ObjectManager() {}

ObjectManager::~ObjectManager() {}

void ObjectManager::addSubscriber(Subscriber *s) { subscribers.push_back(s); }

void ObjectManager::addProcessForest(PForest *pf) { processForest = pf; }

void ObjectManager::addInitialState(ExecutionState *state) {
  states.insert(state);
  processForest->addRoot(state);
}

void ObjectManager::setCurrentState(ExecutionState *_current) {
  assert(current == nullptr);
  current = _current;
  statesUpdated = true;
}

ExecutionState *ObjectManager::branchState(ExecutionState *state,
                                           BranchType reason) {
  assert(statesUpdated);
  ExecutionState *newState = state->branch();
  addedStates.push_back(newState);
  processForest->attach(state->ptreeNode, newState, state);
  stats::incBranchStat(reason, 1);
  return newState;
}

void ObjectManager::removeState(ExecutionState *state) {
  assert(std::find(removedStates.begin(), removedStates.end(), state) ==
         removedStates.end());

  if (!statesUpdated) {
    statesUpdated = true;
  }

  removedStates.push_back(state);
}

void ObjectManager::updateSubscribers() {
  if (statesUpdated) {
    ref<Event> e = new States(current, addedStates, removedStates);
    for (auto s : subscribers) {
      s->update(e);
    }

    for (auto state : addedStates) {
      states.insert(state);
    }

    for (auto state : removedStates) {
      processForest->remove(state->ptreeNode);
      states.erase(state);
      delete state;
    }

    current = nullptr;
    addedStates.clear();
    removedStates.clear();
    statesUpdated = false;
  }
}

void ObjectManager::initialUpdate() {
  addedStates.insert(addedStates.begin(), states.begin(), states.end());
  statesUpdated = true;
  updateSubscribers();
}

const states_ty &ObjectManager::getStates() { return states; }
