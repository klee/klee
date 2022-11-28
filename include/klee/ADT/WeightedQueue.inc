//===- WeightedQueue.inc - --*- C++ -*-------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>
namespace klee {

template <class T, class Comparator>
bool WeightedQueue<T, Comparator>::empty() const {
  return valueToWeight.empty();
}

template <class T, class Comparator>
void WeightedQueue<T, Comparator>::insert(T item, weight_type weight) {
  assert(valueToWeight.count(item) == 0);
  valueToWeight[item] = weight;
  std::vector<T> &weightQueue = weightToQueue[weight];
  weightQueue.push_back(item);
}

template <class T, class Comparator>
void WeightedQueue<T, Comparator>::remove(T item) {
  assert(valueToWeight.count(item) != 0);
  weight_type weight = valueToWeight[item];
  valueToWeight.erase(item);
  std::vector<T> &weightQueue = weightToQueue.at(weight);
  auto itr = std::find(weightQueue.begin(), weightQueue.end(), item);
  assert(itr != weightQueue.end());
  weightQueue.erase(itr);
  if (weightQueue.empty()) {
    weightToQueue.erase(weight);
  }
}

template <class T, class Comparator>
void WeightedQueue<T, Comparator>::update(T item, weight_type weight) {
  assert(valueToWeight.count(item) != 0);
  weight_type oldWeight = valueToWeight[item];
  if (oldWeight != weight) {
    valueToWeight[item] = weight;
    std::vector<T> *weightQueue = &weightToQueue.at(oldWeight);
    auto itr = std::find(weightQueue->begin(), weightQueue->end(), item);
    assert(itr != weightQueue->end());
    weightQueue->erase(itr);
    if (weightQueue->empty()) {
      weightQueue = nullptr;
      weightToQueue.erase(oldWeight);
    }
    weightToQueue[weight].push_back(item);
  }
}

template <class T, class Comparator>
T WeightedQueue<T, Comparator>::choose(
    WeightedQueue<T, Comparator>::weight_type p) {
  if (weightToQueue.empty()) {
    assert(0 && "choose: choose() called on empty queue");
  }
  weight_type maxW = maxWeight();
  if (p >= maxW) {
    auto result = weightToQueue.begin()->second.front();
    return result;
  }

  for (auto weightQueue = weightToQueue.begin(), end = weightToQueue.end();
       weightQueue != end; ++weightQueue) {
    if (p <= weightQueue->first) {
      auto result = weightQueue->second.front();
      return result;
    }
  }
  assert(0 && "unreachable");
}

template <class T, class Comparator>
bool WeightedQueue<T, Comparator>::contains(T item) {
  return valueToWeight.count(item) != 0;
}

template <class T, class Comparator>
bool WeightedQueue<T, Comparator>::tryGetWeight(T item, weight_type &weight) {
  if (valueToWeight.count(item)) {
    weight = valueToWeight[item];
    return true;
  }
  return false;
}

template <class T, class Comparator>
typename WeightedQueue<T, Comparator>::weight_type
WeightedQueue<T, Comparator>::minWeight() {
  if (weightToQueue.empty()) {
    return 0;
  }
  return weightToQueue.begin()->first;
}

template <class T, class Comparator>
typename WeightedQueue<T, Comparator>::weight_type
WeightedQueue<T, Comparator>::maxWeight() {
  if (weightToQueue.empty()) {
    return 0;
  }
  return (--weightToQueue.end())->first;
}

} // namespace klee
