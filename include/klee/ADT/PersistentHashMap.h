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

namespace klee {

template <class K, class D, class HASH = std::hash<K>,
          class EQUAL = std::equal_to<K>>
class PersistentHashMap {
public:
  typedef immer::map<K, D, HASH, EQUAL> Map;
  typedef typename Map::iterator iterator;
  typedef K key_type;
  typedef std::pair<K, D> value_type;

private:
  Map elts;

  PersistentHashMap(const Map &b) : elts(b) {}

public:
  PersistentHashMap() {}
  PersistentHashMap(const PersistentHashMap &b) : elts(b.elts) {}
  ~PersistentHashMap() {}

  PersistentHashMap &operator=(const PersistentHashMap &b) {
    elts = b.elts;
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
      elts = elts.insert(value);
    }
  }
  void replace(const value_type &value) { elts = elts.insert(value); }
  void remove(const key_type &key) { elts = elts.erase(key); }

  iterator begin() const { return elts.begin(); }
  iterator end() const { return elts.end(); }

  const D &operator[](const key_type &key) {
    return elts[key];
    // auto value = lookup(key);
    // if (value) {
    //   return *value;
    // } else {
    //   value_type defVal;
    //   defVal.first = key;
    //   insert(defVal);
    //   return *lookup(key);
    // }
  }

  const D &at(const key_type &key) const { return elts.at(key); }

  void clear() { elts = Map(); }
};
} // namespace klee

#endif /* KLEE_PRESISTENTHASHMAP_H */
