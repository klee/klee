#include "SeedMap.h"
#include "ObjectManager.h"

using namespace klee;

SeedMap::SeedMap() {}

void SeedMap::update(ref<ObjectManager::Event> e) {
  if (auto statesEvent = dyn_cast<ObjectManager::States>(e)) {
    for (const auto state : statesEvent->removed) {
      std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it =
          seedMap.find(state);
      if (it != seedMap.end()) {
        seedMap.erase(it);
      }
    }
  }
}

std::map<ExecutionState *, std::vector<SeedInfo>>::iterator
SeedMap::upper_bound(ExecutionState *state) {
  return seedMap.upper_bound(state);
}

std::map<ExecutionState *, std::vector<SeedInfo>>::iterator
SeedMap::find(ExecutionState *state) {
  return seedMap.find(state);
}

std::map<ExecutionState *, std::vector<SeedInfo>>::iterator SeedMap::begin() {
  return seedMap.begin();
}

std::map<ExecutionState *, std::vector<SeedInfo>>::iterator SeedMap::end() {
  return seedMap.end();
}

void SeedMap::erase(
    std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it) {
  seedMap.erase(it);
}

void SeedMap::erase(ExecutionState *state) { seedMap.erase(state); }

void SeedMap::push_back(ExecutionState *state,
                        std::vector<SeedInfo>::iterator siit) {
  seedMap[state].push_back(*siit);
}

std::size_t SeedMap::count(ExecutionState *state) {
  return seedMap.count(state);
}

std::vector<SeedInfo> &SeedMap::at(ExecutionState *state) {
  return seedMap[state];
}

bool SeedMap::empty() { return seedMap.empty(); }

SeedMap::~SeedMap() {}
