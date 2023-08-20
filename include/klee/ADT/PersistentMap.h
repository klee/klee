//===-- PersistentMap.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_PRESISTENTMAP_H
#define KLEE_PRESISTENTMAP_H

#include <functional>

#include "ImmutableMap.h"

namespace klee {

template <class K, class D, class CMP = std::less<K>> class PersistentMap {
public:
  typedef ImmutableMap<K, D, CMP> Map;
  typedef typename Map::iterator iterator;
  typedef K key_type;
  typedef std::pair<K, D> value_type;

private:
  Map elts;

  PersistentMap(const Map &b) : elts(b) {}

public:
  PersistentMap() {}
  PersistentMap(const PersistentMap &b) : elts(b.elts) {}
  ~PersistentMap() {}

  PersistentMap &operator=(const PersistentMap &b) {
    elts = b.elts;
    return *this;
  }
  bool operator==(const PersistentMap &b) const { return elts == b.elts; }
  bool operator<(const PersistentMap &b) const { return elts < b.elts; }

  bool empty() const { return elts.empty(); }
  size_t count(const key_type &key) const { return elts.count(key); }
  const value_type *lookup(const key_type &key) const {
    return elts.lookup(key);
  }
  const value_type *lookup_previous(const key_type &key) const {
    return elts.lookup_previous(key);
  }
  const value_type &min() const { return elts.min(); }
  const value_type &max() const { return elts.max(); }
  size_t size() const { return elts.size(); }

  void insert(const value_type &value) { elts = elts.insert(value); }
  void replace(const value_type &value) { elts = elts.replace(value); }
  void remove(const key_type &key) { elts = elts.remove(key); }
  void popMin(const value_type &valueOut) { elts = elts.popMin(valueOut); }
  void popMax(const value_type &valueOut) { elts = elts.popMax(valueOut); }

  iterator begin() const { return elts.begin(); }
  iterator end() const { return elts.end(); }
  iterator find(const key_type &key) const { return elts.find(key); }
  iterator lower_bound(const key_type &key) const {
    return elts.lower_bound(key);
  }
  iterator upper_bound(const key_type &key) const {
    return elts.upper_bound(key);
  }

  const value_type &operator[](const key_type &key) {
    auto value = lookup(key);
    if (value) {
      return *value;
    } else {
      value_type defVal;
      defVal.first = key;
      insert(defVal);
      return *lookup(key);
    }
  }

  const D &at(const key_type &key) const { return elts.at(key); }
};
} // namespace klee

#endif /* KLEE_PRESISTENTMAP_H */
