//===-- WeightedQueue.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_WEIGHTEDQUEUE_H
#define KLEE_WEIGHTEDQUEUE_H

#include <deque>
#include <functional>
#include <map>
#include <unordered_map>

namespace klee {

template <class T, class Comparator = std::less<T>> class WeightedQueue {
  typedef unsigned weight_type;

public:
  WeightedQueue() = default;
  ~WeightedQueue() = default;

  bool empty() const;
  void insert(T item, weight_type weight);
  void update(T item, weight_type newWeight);
  void remove(T item);
  bool contains(T item);
  bool tryGetWeight(T item, weight_type &weight);
  T choose(weight_type p);
  weight_type minWeight();
  weight_type maxWeight();

private:
  std::map<weight_type, std::vector<T>> weightToQueue;
  std::unordered_map<T, weight_type> valueToWeight;
};

} // namespace klee

#include "WeightedQueue.inc"

#endif /* KLEE_WEIGHTEDQUEUE_H */
