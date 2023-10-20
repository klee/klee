//===-- TargetedManager.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to manage everything about targets
//
//===----------------------------------------------------------------------===//

#include "DistanceCalculator.h"

#include "klee/Core/Interpreter.h"
#include "klee/Module/TargetHash.h"

#include <map>
#include <unordered_map>

#ifndef KLEE_TARGETMANAGER_H
#define KLEE_TARGETMANAGER_H

namespace klee {
class TargetCalculator;

using TargetHistoryTargetPair =
    std::pair<ref<const TargetsHistory>, ref<Target>>;
using StatesVector = std::vector<ExecutionState *>;
using StateSet = std::set<ExecutionState *>;

class StateIterable final {
private:
  using v = ExecutionState *;
  const StatesVector *self;
  mutable const StateSet *without = nullptr;
  using vec_it = StatesVector::const_iterator;
  bool shouldDestruct;

  class it : public std::iterator<std::input_iterator_tag, v> {
    vec_it self;
    const vec_it ie;
    const StateSet *without = nullptr;
    void goUntilRealNode() {
      if (!without)
        return;
      for (; self != ie; self++) {
        if (!without->count(*self))
          return;
      }
    }

  public:
    it(vec_it i, vec_it ie, const StateSet *without)
        : self(i), ie(ie), without(without) {
      goUntilRealNode();
    }
    bool operator==(const it &other) const noexcept {
      return self == other.self;
    };
    bool operator!=(const it &other) const noexcept {
      return self != other.self;
    };
    it &operator++() noexcept {
      if (self != ie) {
        self++;
        goUntilRealNode();
      }
      return *this;
    }
    v operator*() const noexcept { return *self; }
  };

public:
  StateIterable() : self(new StatesVector()), shouldDestruct(true) {}
  StateIterable(v s) : self(new StatesVector({s})), shouldDestruct(true) {}
  StateIterable(const StatesVector &v, const StateSet *without)
      : self(&v), without(without), shouldDestruct(false) {}
  StateIterable(const StatesVector &v) : StateIterable(v, nullptr) {}
  StateIterable(const StateSet &s)
      : self(new StatesVector(s.begin(), s.end())), shouldDestruct(true) {}
  ~StateIterable() {
    if (shouldDestruct)
      delete self;
  }
  bool empty() const noexcept {
    return without ? begin() == end() : self->empty();
  }
  it begin() const noexcept { return it(self->begin(), self->end(), without); }
  it end() const noexcept { return it(self->end(), self->end(), without); }

  void setWithout(const StateSet *w) const { without = w; }
  void clearWithout() const { without = nullptr; }
};

class TargetHistoryTargetPairToStatesMap final {
private:
  using k = TargetHistoryTargetPair;
  using cv = StateIterable;
  using p = std::pair<k, cv>;
  using v = StatesVector;
  using t =
      std::unordered_map<k, v, TargetHistoryTargetHash, TargetHistoryTargetCmp>;
  using map_it = t::iterator;
  using map_cit = t::const_iterator;
  t self;
  mutable const StateSet *without = nullptr;

  class it : public std::iterator<std::input_iterator_tag, p> {
  protected:
    map_cit self;
    const StateSet *without = nullptr;

  public:
    it(map_cit i, const StateSet *without) : self(i), without(without) {}
    bool operator==(const it &other) const noexcept {
      return self == other.self;
    };
    bool operator!=(const it &other) const noexcept {
      return self != other.self;
    };
    it &operator++() noexcept {
      self++;
      return *this;
    }
    p operator*() const noexcept {
      return std::make_pair(self->first, StateIterable(self->second, without));
    }
  };

public:
  inline v &operator[](const k &__k) { return self[__k]; }
  inline v &operator[](k &&__k) { return self[std::move(__k)]; }
  inline v &at(const k &__k) { return self.at(__k); }
  inline map_it begin() noexcept { return self.begin(); }
  inline map_it end() noexcept { return self.end(); }

  inline const cv at(const k &__k) const { return self.at(__k); }
  inline it begin() const noexcept { return it(self.begin(), without); };
  inline it end() const noexcept { return it(self.end(), without); };

  void setWithout(const StateSet *w) const { without = w; }
  void clearWithout() const { without = nullptr; }
};

class TargetManagerSubscriber {
public:
  virtual ~TargetManagerSubscriber() = default;

  /// Selects a state for further exploration.
  /// \return The selected state.
  virtual void update(const TargetHistoryTargetPairToStatesMap &added,
                      const TargetHistoryTargetPairToStatesMap &removed) = 0;
};

class TargetManager {
private:
  using StatesSet = std::unordered_set<ExecutionState *>;
  using StateToDistanceMap =
      std::unordered_map<const ExecutionState *, TargetHashMap<DistanceResult>>;
  using TargetForestHistoryTargetSet =
      std::unordered_set<TargetHistoryTargetPair, TargetHistoryTargetHash,
                         TargetHistoryTargetCmp>;

  Interpreter::GuidanceKind guidance;
  DistanceCalculator &distanceCalculator;
  TargetCalculator &targetCalculator;
  TargetHashSet reachedTargets;
  StatesSet states;
  StateToDistanceMap distances;
  StatesSet localStates;
  StatesSet changedStates;
  std::vector<TargetManagerSubscriber *> subscribers;
  TargetHistoryTargetPairToStatesMap addedTStates;
  TargetHistoryTargetPairToStatesMap removedTStates;
  TargetHashSet removedTargets;
  TargetHashSet addedTargets;

  void setTargets(ExecutionState &state, const TargetHashSet &targets) {
    state.setTargets(targets);
    changedStates.insert(&state);
  }

  void setHistory(ExecutionState &state, ref<TargetsHistory> history) {
    state.setHistory(history);
    changedStates.insert(&state);
  }

  void updateMiss(ExecutionState &state, ref<Target> target);

  void updateContinue(ExecutionState &state, ref<Target> target);

  void updateDone(ExecutionState &state, ref<Target> target);

  void updateReached(ExecutionState &state);

  void updateTargets(ExecutionState &state);

  void collect(ExecutionState &state);

  bool isReachedTarget(const ExecutionState &state, ref<Target> target,
                       WeightResult &result);

public:
  TargetManager(Interpreter::GuidanceKind _guidance,
                DistanceCalculator &_distanceCalculator,
                TargetCalculator &_targetCalculator)
      : guidance(_guidance), distanceCalculator(_distanceCalculator),
        targetCalculator(_targetCalculator){};

  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates);

  DistanceResult distance(const ExecutionState &state, ref<Target> target) {

    WeightResult wresult;
    if (isReachedTarget(state, target, wresult)) {
      return DistanceResult(wresult);
    }

    if (!state.isTransfered() && distances[&state].count(target)) {
      return distances[&state][target];
    }

    DistanceResult result =
        distanceCalculator.getDistance(state, target->getBlock());

    if (Done == result.result && (!isa<ReachBlockTarget>(target) ||
                                  cast<ReachBlockTarget>(target)->isAtEnd())) {
      result.result = Continue;
    }

    distances[&state][target] = result;

    return result;
  }

  const PersistentSet<ref<Target>> &targets(const ExecutionState &state) {
    return state.targets();
  }

  ref<const TargetsHistory> history(const ExecutionState &state) {
    return state.history();
  }

  ref<const TargetsHistory> prevHistory(const ExecutionState &state) {
    return state.prevHistory();
  }

  const PersistentSet<ref<Target>> &prevTargets(const ExecutionState &state) {
    return state.prevTargets();
  }

  TargetForest &targetForest(ExecutionState &state) {
    return state.targetForest;
  }

  void subscribe(TargetManagerSubscriber &subscriber) {
    subscribers.push_back(&subscriber);
  }

  bool isTargeted(const ExecutionState &state) { return state.isTargeted(); }

  bool isReachedTarget(const ExecutionState &state, ref<Target> target);

  void setReached(ref<Target> target) { reachedTargets.insert(target); }
};

} // namespace klee

#endif /* KLEE_TARGETMANAGER_H */
