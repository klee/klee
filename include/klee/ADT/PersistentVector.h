//===-- PersistentVector.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_PRESISTENTVECTOR_H
#define KLEE_PRESISTENTVECTOR_H

#ifndef IMMER_NO_EXCEPTIONS
#define IMMER_NO_EXCEPTIONS
#endif /* IMMER_NO_EXCEPTIONS */

#include <immer/vector.hpp>

namespace klee {

template <class T> class PersistentVector {
public:
  using Vector = immer::vector<T>;
  using iterator = typename Vector::iterator;
  using value_type = T;

private:
  Vector elts;

  PersistentVector(const Vector &b) : elts(b) {}

public:
  PersistentVector() {}
  PersistentVector(const PersistentVector &b) : elts(b.elts) {}
  ~PersistentVector() {}

  PersistentVector &operator=(const PersistentVector &b) {
    elts = b.elts;
    return *this;
  }

  bool operator==(const PersistentVector &b) const { return elts == b.elts; }
  bool operator!=(const PersistentVector &other) const {
    return !(*this == other);
  }
  bool empty() const { return elts.empty(); }
  size_t size() const { return elts.size(); }

  void set(size_t index, const value_type &value) {
    elts = elts.set(index, value);
  }
  void init(const size_t &n) { elts = Vector(n); }
  void push_back(const value_type &value) { elts = elts.push_back(value); }
  const value_type &back() const { return elts.back(); }
  const value_type &front() const { return elts.front(); }
  const value_type &operator[](const size_t &index) const {
    return elts[index];
  }
  const value_type &at(const size_t &index) const { return elts[index]; }

  iterator begin() const { return elts.begin(); }
  iterator end() const { return elts.end(); }
};
} // namespace klee

#endif /* KLEE_PRESISTENTVECTOR_H */
