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
#include "klee/Core/TargetedExecutionReporter.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"
#include "klee/Module/TargetHash.h"

#include <sstream>
#include <unordered_map>
#include <vector>

namespace klee {
struct TargetHash;
struct TargetCmp;

using KBlockTrace = std::vector<KBlockSet>;

class TargetsHistory {
private:
  unsigned hashValue;
  unsigned sizeValue;

  explicit TargetsHistory(ref<Target> _target,
                          ref<TargetsHistory> _visitedTargets)
      : target(_target), visitedTargets(_visitedTargets) {
    computeHash();
    computeSize();
  }

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

  void computeSize() {
    unsigned res = 0;
    if (target) {
      ++res;
    }
    if (visitedTargets) {
      res += visitedTargets->size();
    }
    sizeValue = res;
  }

protected:
  friend class ref<TargetsHistory>;
  friend class ref<const TargetsHistory>;

  struct TargetsHistoryHash {
    unsigned operator()(TargetsHistory *const t) const { return t->hash(); }
  };

  struct TargetsHistoryCmp {
    bool operator()(TargetsHistory *const a, TargetsHistory *const b) const {
      return a->equals(*b);
    }
  };

  typedef std::unordered_set<TargetsHistory *, TargetsHistoryHash,
                             TargetsHistoryCmp>
      CacheType;

  struct TargetHistoryCacheSet {
    CacheType cache;
    ~TargetHistoryCacheSet() {
      while (cache.size() != 0) {
        ref<TargetsHistory> tmp = *cache.begin();
        tmp->isCached = false;
        cache.erase(cache.begin());
      }
    }
  };

  static TargetHistoryCacheSet cachedHistories;
  bool isCached = false;
  bool toBeCleared = false;

  /// @brief Required by klee::ref-managed objects
  mutable class ReferenceCounter _refCount;

public:
  const ref<Target> target;
  const ref<TargetsHistory> visitedTargets;

  static ref<TargetsHistory> create(ref<Target> _target,
                                    ref<TargetsHistory> _visitedTargets);
  static ref<TargetsHistory> create(ref<Target> _target);
  static ref<TargetsHistory> create();

  ref<TargetsHistory> add(ref<Target> _target) {
    return TargetsHistory::create(_target, this);
  }

  unsigned hash() const { return hashValue; }
  unsigned size() const { return sizeValue; }

  int compare(const TargetsHistory &h) const;
  bool equals(const TargetsHistory &h) const;

  void dump() const;

  ~TargetsHistory();
};

using TargetHistoryTargetPair =
    std::pair<ref<const TargetsHistory>, ref<Target>>;

class TargetForest {
public:
  class UnorderedTargetsSet {
  public:
    static ref<UnorderedTargetsSet> create(const ref<Target> &target);
    static ref<UnorderedTargetsSet> create(const TargetHashSet &targets);

    ~UnorderedTargetsSet();
    unsigned hash() const { return hashValue; }
    bool operator==(const UnorderedTargetsSet &other) const;
    bool operator<(const UnorderedTargetsSet &other) const;

    int compare(const UnorderedTargetsSet &h) const;
    bool equals(const UnorderedTargetsSet &h) const;

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

  protected:
    struct UnorderedTargetsSetHash {
      unsigned operator()(UnorderedTargetsSet *const t) const {
        return t->hash();
      }
    };

    struct UnorderedTargetsSetCmp {
      bool operator()(UnorderedTargetsSet *const a,
                      UnorderedTargetsSet *const b) const {
        return a->equals(*b);
      }
    };

    typedef std::unordered_set<UnorderedTargetsSet *, UnorderedTargetsSetHash,
                               UnorderedTargetsSetCmp>
        CacheType;

    struct UnorderedTargetsSetCacheSet {
      CacheType cache;
      ~UnorderedTargetsSetCacheSet() {
        while (cache.size() != 0) {
          ref<UnorderedTargetsSet> tmp = *cache.begin();
          tmp->isCached = false;
          cache.erase(cache.begin());
        }
      }
    };

    static UnorderedTargetsSetCacheSet cachedVectors;
    bool isCached = false;
    bool toBeCleared = false;

  private:
    explicit UnorderedTargetsSet(const ref<Target> &target);
    explicit UnorderedTargetsSet(const TargetHashSet &targets);
    static ref<UnorderedTargetsSet> create(ref<UnorderedTargetsSet> vec);

    std::vector<ref<Target>> targetsVec;
    unsigned hashValue;

    unsigned sortAndComputeHash();
  };

  struct UnorderedTargetsSetHash {
    unsigned operator()(const ref<TargetForest::UnorderedTargetsSet> &t) const {
      return t->hash();
    }
  };

  struct UnorderedTargetsSetCmp {
    bool operator()(const ref<TargetForest::UnorderedTargetsSet> &a,
                    const ref<TargetForest::UnorderedTargetsSet> &b) const {
      return a == b;
    }
  };

private:
  using TargetToStateSetMap =
      std::unordered_map<ref<Target>, std::unordered_set<ExecutionState *>,
                         TargetHash, TargetCmp>;
  class Layer {
    using InternalLayer =
        std::unordered_map<ref<UnorderedTargetsSet>, ref<Layer>,
                           UnorderedTargetsSetHash, UnorderedTargetsSetCmp>;
    InternalLayer forest;
    using TargetsToVector = std::unordered_map<
        ref<Target>,
        std::unordered_set<ref<UnorderedTargetsSet>, UnorderedTargetsSetHash,
                           UnorderedTargetsSetCmp>,
        TargetHash, TargetCmp>;
    TargetsToVector targetsToVector;

    /// @brief Confidence in % that this layer (i.e., parent target node) can be
    /// reached
    confidence::ty confidence;

    Layer(const InternalLayer &forest, const TargetsToVector &targetsToVector,
          confidence::ty confidence)
        : forest(forest), targetsToVector(targetsToVector),
          confidence(confidence) {}

    explicit Layer(const Layer *layer)
        : Layer(layer->forest, layer->targetsToVector, layer->confidence) {}
    explicit Layer(const ref<Layer> layer) : Layer(layer.get()) {}
    void unionWith(Layer *other);
    void block(ref<Target> target);
    void removeTarget(ref<Target> target);
    Layer *removeChild(ref<UnorderedTargetsSet> child) const;

    confidence::ty getConfidence(confidence::ty parentConfidence) const {
      return confidence::min(parentConfidence, confidence);
    }

  public:
    using iterator = TargetsToVector::const_iterator;

    /// @brief Required by klee::ref-managed objects
    class ReferenceCounter _refCount;

    explicit Layer() : confidence(confidence::MaxConfidence) {}

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
    Layer *replaceChildWith(const std::unordered_set<
                            ref<UnorderedTargetsSet>, UnorderedTargetsSetHash,
                            UnorderedTargetsSetCmp> &other) const;
    Layer *replaceChildWith(ref<UnorderedTargetsSet> child, Layer *other) const;
    Layer *removeChild(ref<Target> child) const;
    Layer *addChild(ref<Target> child) const;
    Layer *addChild(ref<UnorderedTargetsSet> child) const;
    bool hasChild(ref<UnorderedTargetsSet> child) const;
    Layer *blockLeafInChild(ref<UnorderedTargetsSet> child,
                            ref<Target> leaf) const;
    Layer *blockLeafInChild(ref<Target> child, ref<Target> leaf) const;
    Layer *blockLeaf(ref<Target> leaf) const;
    TargetHashSet getTargets() const {
      TargetHashSet targets;
      for (auto &targetVect : targetsToVector) {
        targets.insert(targetVect.first);
      }
      return targets;
    }

    void dump(unsigned n) const;
    void pullConfidences(
        std::vector<std::pair<ref<UnorderedTargetsSet>, confidence::ty>> &leafs,
        confidence::ty parentConfidence) const;
    void pullLeafs(std::vector<ref<Target>> &leafs) const;
    void propagateConfidenceToChildren();
    ref<Layer> deepCopy();
    Layer *copy();
    void divideConfidenceBy(unsigned factor);
    Layer *
    divideConfidenceBy(const TargetToStateSetMap &reachableStatesOfTarget);
    confidence::ty getConfidence() const {
      return getConfidence(confidence::MaxConfidence);
    }

    void
    addTrace(const Result &result,
             const std::unordered_map<ref<Location>, KBlockSet, RefLocationHash,
                                      RefLocationCmp> &locToBlocks);
  };

  ref<Layer> forest;

private:
  ref<TargetsHistory> history;
  KFunction *entryFunction;

public:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;
  KFunction *getEntryFunction() { return entryFunction; }

  void
  addTrace(const Result &result,
           const std::unordered_map<ref<Location>, KBlockSet, RefLocationHash,
                                    RefLocationCmp> &locToBlocks) {
    forest->addTrace(result, locToBlocks);
  }

  TargetForest(ref<Layer> layer, KFunction *entryFunction)
      : forest(layer), history(TargetsHistory::create()),
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
  void add(ref<UnorderedTargetsSet>);
  void remove(ref<Target>);
  void blockIn(ref<Target>, ref<Target>);
  void block(const ref<Target> &);
  const ref<TargetsHistory> getHistory() { return history; };
  const ref<Layer> getTopLayer() { return forest; };
  const TargetHashSet getTargets() const { return forest->getTargets(); }
  void dump() const;
  std::vector<std::pair<ref<UnorderedTargetsSet>, confidence::ty>>
  confidences() const;
  std::set<ref<Target>> leafs() const;
  ref<TargetForest> deepCopy();
  void divideConfidenceBy(unsigned factor) {
    forest->divideConfidenceBy(factor);
  }
  void divideConfidenceBy(const TargetToStateSetMap &reachableStatesOfTarget) {
    forest = forest->divideConfidenceBy(reachableStatesOfTarget);
  }
};

struct TargetsHistoryHash {
  unsigned operator()(const ref<TargetsHistory> &t) const { return t->hash(); }
};

struct TargetsHistoryCmp {
  bool operator()(const ref<TargetsHistory> &a,
                  const ref<TargetsHistory> &b) const {
    return a == b;
  }
};

struct TargetHistoryTargetHash {
  unsigned operator()(const TargetHistoryTargetPair &t) const {
    return t.first->hash() * Expr::MAGIC_HASH_CONSTANT + t.second->hash();
  }
};

struct TargetHistoryTargetCmp {
  bool operator()(const TargetHistoryTargetPair &a,
                  const TargetHistoryTargetPair &b) const {
    return a.first == b.first && a.second == b.second;
  }
};

struct UnorderedTargetsSetHash {
  std::size_t operator()(const TargetForest::UnorderedTargetsSet *t) const {
    return t->hash();
  }
};

struct UnorderedTargetsSetCmp {
  bool operator()(const TargetForest::UnorderedTargetsSet *a,
                  const TargetForest::UnorderedTargetsSet *b) const {
    return a == b;
  }
};

struct EquivUnorderedTargetsSetCmp {
  bool operator()(const TargetForest::UnorderedTargetsSet *a,
                  const TargetForest::UnorderedTargetsSet *b) const {
    if (a == NULL || b == NULL)
      return false;
    return *a == *b;
  }
};

} // namespace klee

#endif /* KLEE_TARGETFOREST_H */
