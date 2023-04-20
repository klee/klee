//===-- TargetForest.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to represent prefix tree of Targets
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TARGETFOREST_H
#define KLEE_TARGETFOREST_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"
#include "klee/Module/TargetHash.h"

#include <sstream>
#include <unordered_map>
#include <vector>

namespace klee {
struct RefTargetHash;
struct RefTargetCmp;
struct TargetsHistoryHash;
struct EquivTargetsHistoryCmp;
struct TargetsHistoryCmp;
struct TargetsVectorHash;
struct TargetsVectorCmp;
struct EquivTargetsVectorCmp;
struct RefTargetsVectorHash;
struct RefTargetsVectorCmp;

class TargetForest {
public:
  using TargetsSet =
      std::unordered_set<ref<Target>, RefTargetHash, RefTargetCmp>;

  class UnorderedTargetsSet {
  public:
    static ref<UnorderedTargetsSet> create(const ref<Target> &target);
    static ref<UnorderedTargetsSet> create(const TargetsSet &targets);

    ~UnorderedTargetsSet();
    std::size_t hash() const { return hashValue; }
    bool operator==(const UnorderedTargetsSet &other) const {
      return targetsVec == other.targetsVec;
    }

    const std::vector<ref<Target>> &getTargets() const { return targetsVec; }

    std::string toString() const {
      std::stringstream ss;
      for (const auto &target : getTargets()) {
        ss << target->toString() << '\n';
      }
      return ss.str();
    }

    /// @brief Required by klee::ref-managed objects
    class ReferenceCounter _refCount;

  private:
    explicit UnorderedTargetsSet(const ref<Target> &target);
    explicit UnorderedTargetsSet(const TargetsSet &targets);
    static ref<UnorderedTargetsSet> create(UnorderedTargetsSet *vec);

    typedef std::unordered_set<UnorderedTargetsSet *, TargetsVectorHash,
                               EquivTargetsVectorCmp>
        EquivTargetsVectorHashSet;
    typedef std::unordered_set<UnorderedTargetsSet *, TargetsVectorHash,
                               TargetsVectorCmp>
        TargetsVectorHashSet;

    static EquivTargetsVectorHashSet cachedVectors;
    static TargetsVectorHashSet vectors;
    std::vector<ref<Target>> targetsVec;
    std::size_t hashValue;

    void sortAndComputeHash();
  };

  struct RefTargetsVectorHash {
    unsigned operator()(const ref<TargetForest::UnorderedTargetsSet> &t) const {
      return t->hash();
    }
  };

  struct RefTargetsVectorCmp {
    bool operator()(const ref<TargetForest::UnorderedTargetsSet> &a,
                    const ref<TargetForest::UnorderedTargetsSet> &b) const {
      return a.get() == b.get();
    }
  };

private:
  class Layer {
    using InternalLayer =
        std::unordered_map<ref<UnorderedTargetsSet>, ref<Layer>,
                           RefTargetsVectorHash, RefTargetsVectorCmp>;
    InternalLayer forest;
    using TargetsToVector = std::unordered_map<
        ref<Target>,
        std::unordered_set<ref<UnorderedTargetsSet>, RefTargetsVectorHash,
                           RefTargetsVectorCmp>,
        RefTargetHash, RefTargetCmp>;
    TargetsToVector targetsToVector;

    Layer(const InternalLayer &forest, const TargetsToVector &targetsToVector)
        : forest(forest), targetsToVector(targetsToVector) {}
    explicit Layer(const Layer *layer)
        : Layer(layer->forest, layer->targetsToVector) {}
    explicit Layer(const ref<Layer> layer) : Layer(layer.get()) {}
    void unionWith(Layer *other);
    void block(ref<Target> target);
    void removeTarget(ref<Target> target);
    Layer *removeChild(ref<UnorderedTargetsSet> child) const;

    void collectHowManyEventsInTracesWereReached(
        std::unordered_map<unsigned, std::pair<unsigned, unsigned>>
            &traceToEventCount,
        unsigned reached, unsigned total) const;

  public:
    using iterator = TargetsToVector::const_iterator;

    /// @brief Required by klee::ref-managed objects
    class ReferenceCounter _refCount;

    explicit Layer() {}

    iterator find(ref<Target> b) const { return targetsToVector.find(b); }
    iterator begin() const { return targetsToVector.begin(); }
    iterator end() const { return targetsToVector.end(); }
    void insert(ref<UnorderedTargetsSet> loc, ref<Layer> nextLayer) {
      forest[loc] = nextLayer;
    }
    void insertTargetsToVec(ref<Target> target,
                            ref<UnorderedTargetsSet> targetsVec) {
      targetsToVector[target].insert(targetsVec);
    }
    bool empty() const { return forest.empty(); }
    bool deepFind(ref<Target> target) const;
    bool deepFindIn(ref<Target> child, ref<Target> target) const;
    size_t size() const { return forest.size(); }
    Layer *replaceChildWith(
        ref<Target> child,
        const std::unordered_set<ref<UnorderedTargetsSet>, RefTargetsVectorHash,
                                 RefTargetsVectorCmp> &other) const;
    Layer *replaceChildWith(ref<UnorderedTargetsSet> child, Layer *other) const;
    Layer *removeChild(ref<Target> child) const;
    Layer *addChild(ref<Target> child) const;
    Layer *blockLeafInChild(ref<UnorderedTargetsSet> child,
                            ref<Target> leaf) const;
    Layer *blockLeafInChild(ref<Target> child, ref<Target> leaf) const;
    Layer *blockLeaf(ref<Target> leaf) const;
    bool allNodesRefCountOne() const;
    void dump(unsigned n) const;
    ref<Layer> deepCopy();
    Layer *copy();
    void collectHowManyEventsInTracesWereReached(
        std::unordered_map<unsigned, std::pair<unsigned, unsigned>>
            &traceToEventCount) const {
      collectHowManyEventsInTracesWereReached(traceToEventCount, 0, 0);
    }

    void addTrace(
        const Result &result,
        const std::unordered_map<ref<Location>, std::unordered_set<KBlock *>,
                                 RefLocationHash, RefLocationCmp> &locToBlocks);
  };

  ref<Layer> forest;

  bool allNodesRefCountOne() const;

public:
  class History {
  private:
    typedef std::unordered_set<History *, TargetsHistoryHash,
                               EquivTargetsHistoryCmp>
        EquivTargetsHistoryHashSet;
    typedef std::unordered_set<History *, TargetsHistoryHash, TargetsHistoryCmp>
        TargetsHistoryHashSet;

    static EquivTargetsHistoryHashSet cachedHistories;
    static TargetsHistoryHashSet histories;
    unsigned hashValue;

    explicit History(ref<Target> _target, ref<History> _visitedTargets)
        : target(_target), visitedTargets(_visitedTargets) {
      computeHash();
    }

  public:
    const ref<Target> target;
    const ref<History> visitedTargets;

    static ref<History> create(ref<Target> _target,
                               ref<History> _visitedTargets);
    static ref<History> create(ref<Target> _target);
    static ref<History> create();

    ref<History> add(ref<Target> _target) {
      return History::create(_target, this);
    }

    unsigned hash() const { return hashValue; }

    int compare(const History &h) const;
    bool equals(const History &h) const;

    void computeHash() {
      unsigned res = 0;
      if (target) {
        res = target->hash() * Expr::MAGIC_HASH_CONSTANT;
      }
      if (visitedTargets) {
        res ^= visitedTargets->hash() * Expr::MAGIC_HASH_CONSTANT;
      }
      hashValue = res;
    }

    void dump() const;

    ~History();

    /// @brief Required by klee::ref-managed objects
    class ReferenceCounter _refCount;
  };

private:
  ref<History> history;
  KFunction *entryFunction;

public:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;
  unsigned getDebugReferenceCount() { return forest->_refCount.getCount(); }
  KFunction *getEntryFunction() { return entryFunction; }

  void addTrace(
      const Result &result,
      const std::unordered_map<ref<Location>, std::unordered_set<KBlock *>,
                               RefLocationHash, RefLocationCmp> &locToBlocks) {
    forest->addTrace(result, locToBlocks);
  }

  TargetForest(ref<Layer> layer, KFunction *entryFunction)
      : forest(layer), history(History::create()),
        entryFunction(entryFunction) {}
  TargetForest() : TargetForest(new Layer(), nullptr) {}
  TargetForest(KFunction *entryFunction)
      : TargetForest(new Layer(), entryFunction) {}

  bool empty() const { return forest.isNull() || forest->empty(); }
  Layer::iterator begin() const { return forest->begin(); }
  Layer::iterator end() const { return forest->end(); }
  bool deepContains(ref<Target> b) { return forest->deepFind(b); }
  bool contains(ref<Target> b) { return forest->find(b) != forest->end(); }

  /// @brief Number of children of this layer (immediate successors)
  size_t successorCount() const { return forest->size(); }

  void stepTo(ref<Target>);
  void add(ref<Target>);
  void remove(ref<Target>);
  void blockIn(ref<Target>, ref<Target>);
  const ref<History> getHistory() { return history; };
  const ref<Layer> getTargets() { return forest; };
  void dump() const;
  ref<TargetForest> deepCopy();
  void collectHowManyEventsInTracesWereReached(
      std::unordered_map<unsigned, std::pair<unsigned, unsigned>>
          &traceToEventCount) const {
    forest->collectHowManyEventsInTracesWereReached(traceToEventCount);
  }
};

struct TargetsHistoryHash {
  unsigned operator()(const TargetForest::History *t) const {
    return t ? t->hash() : 0;
  }
};

struct TargetsHistoryCmp {
  bool operator()(const TargetForest::History *a,
                  const TargetForest::History *b) const {
    return a == b;
  }
};

struct EquivTargetsHistoryCmp {
  bool operator()(const TargetForest::History *a,
                  const TargetForest::History *b) const {
    if (a == NULL || b == NULL)
      return false;
    return a->compare(*b) == 0;
  }
};

struct RefTargetsHistoryHash {
  unsigned operator()(const ref<TargetForest::History> &t) const {
    return t->hash();
  }
};

struct RefTargetsHistoryCmp {
  bool operator()(const ref<TargetForest::History> &a,
                  const ref<TargetForest::History> &b) const {
    return a.get() == b.get();
  }
};

struct TargetsVectorHash {
  std::size_t operator()(const TargetForest::UnorderedTargetsSet *t) const {
    return t->hash();
  }
};

struct TargetsVectorCmp {
  bool operator()(const TargetForest::UnorderedTargetsSet *a,
                  const TargetForest::UnorderedTargetsSet *b) const {
    return a == b;
  }
};

struct EquivTargetsVectorCmp {
  bool operator()(const TargetForest::UnorderedTargetsSet *a,
                  const TargetForest::UnorderedTargetsSet *b) const {
    if (a == NULL || b == NULL)
      return false;
    return *a == *b;
  }
};

} // namespace klee

#endif /* KLEE_TARGETFOREST_H */
