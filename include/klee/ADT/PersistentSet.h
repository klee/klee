//===-- PersistentSet.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_PRESISTENTSET_H
#define KLEE_PRESISTENTSET_H

#include <functional>

#include "ImmutableSet.h"

namespace klee {

template <class T, class CMP = std::less<T>> class PersistentSet {
public:
  typedef ImmutableSet<T, CMP> Set;
  typedef typename Set::iterator iterator;
  typedef T key_type;
  typedef T value_type;

private:
  Set elts;

  PersistentSet(const Set &b) : elts(b) {}

public:
  PersistentSet() {}
  PersistentSet(const PersistentSet &b) : elts(b.elts) {}
  ~PersistentSet() {}

  PersistentSet &operator=(const PersistentSet &b) {
    elts = b.elts;
    return *this;
  }

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

  bool operator==(const PersistentSet &other) const {
    if (size() != other.size()) {
      return false;
    }
    auto it = begin(), otherIt = other.begin(), endIt = end(),
         otherEndIt = other.end();
    for (; it != endIt && otherIt != otherEndIt; ++it, ++otherIt) {
      if (*it != *otherIt) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const PersistentSet &other) const {
    return !(*this == other);
  }

  iterator find(const key_type &key) const { return elts.find(key); }
  iterator lower_bound(const key_type &key) const {
    return elts.lower_bound(key);
  }
  iterator upper_bound(const key_type &key) const {
    return elts.upper_bound(key);
  }
};
} // namespace klee

#endif /* KLEE_PRESISTENTSET_H */
