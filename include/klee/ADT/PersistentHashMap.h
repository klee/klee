//===-- PersistentHashMap.h -------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_PRESISTENTHASHMAP_H
#define KLEE_PRESISTENTHASHMAP_H

#ifndef IMMER_NO_EXCEPTIONS
#define IMMER_NO_EXCEPTIONS
#endif /* IMMER_NO_EXCEPTIONS */

#include <immer/map.hpp>
#include <immer/map_transient.hpp>

namespace klee {

template <class K, class D, class HASH = std::hash<K>,
          class EQUAL = std::equal_to<K>>
class PersistentHashMap {
public:
  typedef immer::map_transient<K, D, HASH, EQUAL> Map;
  typedef typename Map::iterator iterator;
  typedef K key_type;
  typedef std::pair<K, D> value_type;

private:
  mutable Map elts;

  PersistentHashMap(const Map &b) : elts(b.persistent().transient()) {}

public:
  PersistentHashMap() = default;
  PersistentHashMap(const PersistentHashMap &b) {
    elts = b.elts.persistent().transient();
  }
  ~PersistentHashMap() = default;

  PersistentHashMap &operator=(const PersistentHashMap &b) {
    elts = b.elts.persistent().transient();
    return *this;
  }
  bool operator==(const PersistentHashMap &b) const { return elts == b.elts; }
  bool operator<(const PersistentHashMap &b) const { return elts < b.elts; }

  bool empty() const { return elts.empty(); }
  size_t count(const key_type &key) const { return elts.count(key); }
  const D *lookup(const key_type &key) const { return elts.find(key); }
  size_t size() const { return elts.size(); }

  void insert(const value_type &value) {
    if (!lookup(value.first)) {
      elts.insert(value);
    }
  }
  void replace(const value_type &value) { elts.insert(value); }
  void remove(const key_type &key) { elts.erase(key); }

  iterator begin() const { return elts.begin(); }
  iterator end() const { return elts.end(); }

  const D &at(const key_type &key) const { return elts.at(key); }
  const D &operator[](const key_type &key) { return at(key); }

  void clear() { elts = Map(); }
};
} // namespace klee

#endif /* KLEE_PRESISTENTHASHMAP_H */
