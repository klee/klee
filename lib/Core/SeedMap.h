#ifndef KLEE_SEEDMAP_H
#define KLEE_SEEDMAP_H

#include "ExecutionState.h"
#include "ObjectManager.h"
#include "SeedInfo.h"

#include <map>

namespace klee {
class SeedMap : public Subscriber {
private:
  std::map<ExecutionState *, std::vector<SeedInfo>> seedMap;

public:
  SeedMap();

  void update(ref<ObjectManager::Event> e) override;

  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator
  upper_bound(ExecutionState *state);
  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator
  find(ExecutionState *state);
  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator end();
  std::map<ExecutionState *, std::vector<SeedInfo>>::iterator begin();
  void erase(std::map<ExecutionState *, std::vector<SeedInfo>>::iterator it);
  void erase(ExecutionState *state);
  void push_back(ExecutionState *state, std::vector<SeedInfo>::iterator siit);
  std::size_t count(ExecutionState *state);
  std::vector<SeedInfo> &at(ExecutionState *state);
  bool empty();

  virtual ~SeedMap();
};
} // namespace klee

#endif /*KLEE_SEEDMAP_H*/
