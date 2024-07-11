#ifndef KLEE_DISJOINEDSETUNION_H
#define KLEE_DISJOINEDSETUNION_H

#include "klee/ADT/Either.h"
#include "klee/ADT/Ref.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/Symcrete.h"

#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace klee {
using ExprOrSymcrete = either<Expr, Symcrete>;

template <typename ValueType, typename SetType,
          typename HASH = std::hash<ValueType>,
          typename PRED = std::equal_to<ValueType>,
          typename CMP = std::less<ValueType>>
class DisjointSetUnion {
public:
  using internal_storage_ty = std::unordered_set<ValueType, HASH, PRED>;
  using disjoint_sets_ty =
      std::unordered_map<ValueType, ref<const SetType>, HASH, PRED>;
  using iterator = typename internal_storage_ty::const_iterator;

protected:
  std::unordered_map<ValueType, ValueType, HASH, PRED> parent;
  std::set<ValueType, CMP> roots;
  std::unordered_map<ValueType, size_t, HASH, PRED> rank;

  std::unordered_set<ValueType, HASH, PRED> internalStorage;
  std::unordered_map<ValueType, ref<const SetType>, HASH, PRED> disjointSets;

  ValueType find(const ValueType &v) { // findparent
    assert(parent.find(v) != parent.end());
    if (v == parent.at(v))
      return v;
    parent.insert_or_assign(v, find(parent.at(v)));
    return parent.at(v);
  }

  ValueType constFind(const ValueType &v) const {
    assert(parent.find(v) != parent.end());
    ValueType v1 = parent.at(v);
    if (v == v1)
      return v;
    return constFind(v1);
  }

  void merge(ValueType a, ValueType b) {
    a = find(a);
    b = find(b);
    if (a == b) {
      return;
    }

    if (rank.at(a) < rank.at(b)) {
      std::swap(a, b);
    }
    parent.insert_or_assign(b, a);
    if (rank.at(a) == rank.at(b)) {
      rank.insert_or_assign(a, rank.at(a) + 1);
    }

    roots.erase(b);
    disjointSets.insert_or_assign(
        a, SetType::merge(disjointSets.at(a), disjointSets.at(b)));
    disjointSets.erase(b);
  }

  bool areJoined(const ValueType &i, const ValueType &j) const {
    return constFind(i) == constFind(j);
  }

public:
  iterator begin() const { return internalStorage.begin(); }
  iterator end() const { return internalStorage.end(); }

  size_t numberOfValues() const noexcept { return internalStorage.size(); }

  size_t numberOfGroups() const noexcept { return disjointSets.size(); }

  bool empty() const noexcept { return numberOfValues() == 0; }

  ref<const SetType> findGroup(const ValueType &i) const {
    return disjointSets.find(constFind(i))->second;
  }

  ref<const SetType> findGroup(iterator it) const {
    return disjointSets.find(constFind(*it))->second;
  }

  void addValue(const ValueType value) {
    if (internalStorage.find(value) != internalStorage.end()) {
      return;
    }
    parent.insert({value, value});
    roots.insert(value);
    rank.insert({value, 0});
    disjointSets.insert({value, new SetType(value)});

    internalStorage.insert(value);
    std::set<ValueType, CMP> oldRoots = roots;
    for (ValueType v : oldRoots) {
      if (!areJoined(v, value) &&
          SetType::intersects(disjointSets.at(find(v)),
                              disjointSets.at(find(value)))) {
        merge(v, value);
      }
    }
  }
  void getAllIndependentSets(std::vector<ref<const SetType>> &result) const {
    for (ValueType v : roots)
      result.push_back(findGroup(v));
  }

  void add(const DisjointSetUnion &b) {
    std::set<ValueType, CMP> oldRoots = roots;
    std::set<ValueType, CMP> newRoots = b.roots;
    for (auto it : b.parent) {
      parent.insert(it);
    }
    for (auto it : b.roots) {
      roots.insert(it);
    }
    for (auto it : b.rank) {
      rank.insert(it);
    }
    for (auto it : b.internalStorage) {
      internalStorage.insert(it);
    }
    for (auto it : b.disjointSets) {
      disjointSets.insert(it);
    }
    for (ValueType nv : newRoots) {
      for (ValueType ov : oldRoots) {
        if (!areJoined(ov, nv) &&
            SetType::intersects(disjointSets.at(find(ov)),
                                disjointSets.at(find(nv)))) {
          merge(ov, nv);
        }
      }
    }
  }

  DisjointSetUnion() {}

  DisjointSetUnion(const internal_storage_ty &is) {
    for (ValueType v : is) {
      addValue(v);
    }
  }

public:
  internal_storage_ty is() const { return internalStorage; }

  disjoint_sets_ty ds() const { return disjointSets; }
};
} // namespace klee

#endif /* KLEE_DISJOINEDSETUNION_H */
