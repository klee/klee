//===-- Searcher.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SEARCHER_H
#define KLEE_SEARCHER_H

#include "DistanceCalculator.h"
#include "ExecutionState.h"
#include "PForest.h"
#include "PTree.h"
#include "TargetManager.h"

#include "klee/ADT/RNG.h"
#include "klee/Module/KModule.h"
#include "klee/System/Time.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

namespace llvm {
class BasicBlock;
class Function;
class Instruction;
class raw_ostream;
} // namespace llvm

namespace klee {
class DistanceCalculator;
template <class T, class Comparator> class DiscretePDF;
template <class T, class Comparator> class WeightedQueue;
class ExecutionState;
class TargetCalculator;
class TargetForest;
class TargetManagerSubscriber;

/// A Searcher implements an exploration strategy for the Executor by selecting
/// states for further exploration using different strategies or heuristics.
class Searcher {
public:
  virtual ~Searcher() = default;

  /// Selects a state for further exploration.
  /// \return The selected state.
  virtual ExecutionState &selectState() = 0;

  /// Notifies searcher about new or deleted states.
  /// \param current The currently selected state for exploration.
  /// \param addedStates The newly branched states with `current` as common
  /// ancestor. \param removedStates The states that will be terminated.
  virtual void update(ExecutionState *current, const StateIterable &addedStates,
                      const StateIterable &removedStates) = 0;

  /// \return True if no state left for exploration, False otherwise
  virtual bool empty() = 0;

  /// Prints name of searcher as a `klee_message()`.
  // TODO: could probably made prettier or more flexible
  virtual void printName(llvm::raw_ostream &os) = 0;

  enum CoreSearchType : std::uint8_t {
    DFS,
    BFS,
    RandomState,
    RandomPath,
    NURS_CovNew,
    NURS_MD2U,
    NURS_Depth,
    NURS_RP,
    NURS_ICnt,
    NURS_CPICnt,
    NURS_QC
  };
};

/// DFSSearcher implements depth-first exploration. All states are kept in
/// insertion order. The last state is selected for further exploration.
class DFSSearcher final : public Searcher {
  std::vector<ExecutionState *> states;

public:
  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

/// BFSSearcher implements breadth-first exploration. When KLEE branches
/// multiple times for a single instruction, all new states have the same depth.
/// Keep in mind that the process tree (PTree) is a binary tree and hence the
/// depth of a state in that tree and its branch depth during BFS are different.
class BFSSearcher final : public Searcher {
  std::deque<ExecutionState *> states;

public:
  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

/// RandomSearcher picks a state randomly.
class RandomSearcher final : public Searcher {
  std::vector<ExecutionState *> states;
  RNG &theRNG;

public:
  explicit RandomSearcher(RNG &rng);
  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

class TargetedSearcher final : public Searcher {
private:
  std::unique_ptr<WeightedQueue<ExecutionState *, ExecutionStateIDCompare>>
      states;
  ref<Target> target;
  DistanceCalculator &distanceCalculator;

  weight_type getWeight(ExecutionState *es);

public:
  TargetedSearcher(ref<Target> target, DistanceCalculator &distanceCalculator);
  ~TargetedSearcher() override;

  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

class GuidedSearcher final : public Searcher, public TargetManagerSubscriber {
  template <class T>
  using TargetHistoryTargetPairHashMap =
      std::unordered_map<TargetHistoryTargetPair, T, TargetHistoryTargetHash,
                         TargetHistoryTargetCmp>;
  using TargetHistoryTargetPairToSearcherMap =
      std::unordered_map<TargetHistoryTargetPair,
                         std::unique_ptr<TargetedSearcher>,
                         TargetHistoryTargetHash, TargetHistoryTargetCmp>;
  using TargetForestHisoryTargetVector = std::vector<TargetHistoryTargetPair>;
  using TargetForestHistoryTargetSet =
      std::unordered_set<TargetHistoryTargetPair, TargetHistoryTargetHash,
                         TargetHistoryTargetCmp>;

  std::unique_ptr<Searcher> baseSearcher;
  TargetHistoryTargetPairToSearcherMap targetedSearchers;
  DistanceCalculator &distanceCalculator;
  RNG &theRNG;
  unsigned index{1};

  TargetForestHistoryTargetSet localHistoryTargets;
  std::vector<ExecutionState *> baseAddedStates;
  std::vector<ExecutionState *> baseRemovedStates;
  TargetHistoryTargetPairToStatesMap addedTStates;
  TargetHistoryTargetPairToStatesMap removedTStates;

  TargetHashSet removedTargets;
  TargetHashSet addedTargets;
  TargetForestHistoryTargetSet currTargets;

  TargetForestHisoryTargetVector historiesAndTargets;
  bool isThereTarget(ref<const TargetsHistory> history, ref<Target> target);
  void addTarget(ref<const TargetsHistory> history, ref<Target> target);
  void removeTarget(ref<const TargetsHistory> history, ref<Target> target);

public:
  GuidedSearcher(Searcher *baseSearcher, DistanceCalculator &distanceCalculator,
                 RNG &rng)
      : baseSearcher(baseSearcher), distanceCalculator(distanceCalculator),
        theRNG(rng) {}
  ~GuidedSearcher() override = default;
  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  void update(const TargetHistoryTargetPairToStatesMap &added,
              const TargetHistoryTargetPairToStatesMap &removed) override;
  void updateTargets(ExecutionState *current);

  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

/// The base class for all weighted searchers. Uses DiscretePDF as underlying
/// data structure.
class WeightedRandomSearcher final : public Searcher {
public:
  enum WeightType : std::uint8_t {
    Depth,
    RP,
    QueryCost,
    InstCount,
    CPInstCount,
    MinDistToUncovered,
    CoveringNew
  };

private:
  std::unique_ptr<DiscretePDF<ExecutionState *, ExecutionStateIDCompare>>
      states;
  RNG &theRNG;
  WeightType type;
  bool updateWeights;

  double getWeight(ExecutionState *);

public:
  /// \param type The WeightType that determines the underlying heuristic.
  /// \param RNG A random number generator.
  WeightedRandomSearcher(WeightType type, RNG &rng);
  ~WeightedRandomSearcher() override = default;

  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

/// RandomPathSearcher performs a random walk of the PTree to select a state.
/// PTree is a global data structure, however, a searcher can sometimes only
/// select from a subset of all states (depending on the update calls).
///
/// To support this, RandomPathSearcher has a subgraph view of PTree, in that it
/// only walks the PTreeNodes that it "owns". Ownership is stored in the
/// getInt method of the PTreeNodePtr class (which hides it in the pointer
/// itself).
///
/// The current implementation of PTreeNodePtr supports only 3 instances of the
/// RandomPathSearcher. This is because the current PTreeNodePtr implementation
/// conforms to C++ and only steals the last 3 alignment bits. This restriction
/// could be relaxed slightly by an architecture-specific implementation of
/// PTreeNodePtr that also steals the top bits of the pointer.
///
/// The ownership bits are maintained in the update method.
class RandomPathSearcher final : public Searcher {
  PForest &processForest;
  RNG &theRNG;

  // Unique bitmask of this searcher
  const uint8_t idBitMask;

public:
  /// \param processTree The process tree.
  /// \param RNG A random number generator.
  RandomPathSearcher(PForest &processForest, RNG &rng);
  ~RandomPathSearcher() override = default;

  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

/// BatchingSearcher selects a state from an underlying searcher and returns
/// that state for further exploration for a given time or a given number
/// of instructions.
class BatchingSearcher final : public Searcher {
  std::unique_ptr<Searcher> baseSearcher;
  bool timeBudgetEnabled;
  time::Span timeBudget;
  bool instructionBudgetEnabled;
  unsigned instructionBudget;

  ExecutionState *lastState{nullptr};
  time::Point lastStartTime;
  unsigned lastStartInstructions;

  bool withinTimeBudget() const;
  bool withinInstructionBudget() const;

public:
  /// \param baseSearcher The underlying searcher (takes ownership).
  /// \param timeBudget Time span a state gets selected before choosing a
  /// different one. \param instructionBudget Number of instructions to
  /// re-select a state for.
  BatchingSearcher(Searcher *baseSearcher, time::Span timeBudget,
                   unsigned instructionBudget);
  ~BatchingSearcher() override = default;

  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

/// IterativeDeepeningSearcher implements a metric-based deepening. States
/// are selected from an underlying searcher. When a state exceeds its metric
/// limit, it is paused (removed from underlying searcher). When the underlying
/// searcher runs out of states, the metric limit is increased and all paused
/// states are revived (added to underlying searcher).
class IterativeDeepeningSearcher final : public Searcher,
                                         public TargetManagerSubscriber {
public:
  struct Metric {
    virtual ~Metric() = default;
    virtual void selectState(){};
    virtual bool exceeds(const ExecutionState &state) const = 0;
    virtual void increaseLimit() = 0;
  };

private:
  std::unique_ptr<Searcher> baseSearcher;
  TargetManagerSubscriber *tms;
  std::unique_ptr<Metric> metric;
  std::set<ExecutionState *> pausedStates;

public:
  /// \param baseSearcher The underlying searcher (takes ownership).
  explicit IterativeDeepeningSearcher(Searcher *baseSearcher,
                                      TargetManagerSubscriber *tms,
                                      HaltExecution::Reason metric);
  ~IterativeDeepeningSearcher() override = default;

  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  void update(const TargetHistoryTargetPairToStatesMap &added,
              const TargetHistoryTargetPairToStatesMap &removed) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

/// InterleavedSearcher selects states from a set of searchers in round-robin
/// manner. It is used for KLEE's default strategy where it switches between
/// RandomPathSearcher and WeightedRandomSearcher with CoveringNew metric.
class InterleavedSearcher final : public Searcher {
  std::vector<std::unique_ptr<Searcher>> searchers;
  unsigned index{1};

public:
  /// \param searchers The underlying searchers (takes ownership).
  explicit InterleavedSearcher(const std::vector<Searcher *> &searchers);
  ~InterleavedSearcher() override = default;

  ExecutionState &selectState() override;
  void update(ExecutionState *current, const StateIterable &addedStates,
              const StateIterable &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;
};

} // namespace klee

#endif /* KLEE_SEARCHER_H */
