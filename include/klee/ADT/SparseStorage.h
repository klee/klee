#ifndef KLEE_SPARSESTORAGE_H
#define KLEE_SPARSESTORAGE_H

#include <cassert>
#include <cstddef>
#include <iterator>
#include <map>
#include <vector>

namespace klee {

template <typename ValueType> class SparseStorage {
private:
  size_t capacity;
  std::map<size_t, ValueType> internalStorage;
  ValueType defaultValue;

  bool contains(size_t key) const { return internalStorage.count(key) != 0; }

public:
  struct Iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ValueType;
    using pointer = ValueType *;
    using reference = ValueType &;

  private:
    size_t idx;
    const SparseStorage *owner;

  public:
    Iterator(size_t idx, const SparseStorage *owner) : idx(idx), owner(owner) {}

    value_type operator*() const { return owner->load(idx); }

    Iterator &operator++() {
      ++idx;
      return *this;
    }

    Iterator operator++(int) {
      Iterator snap = *this;
      ++(*this);
      return snap;
    }

    bool operator==(const Iterator &other) const { return idx == other.idx; }

    bool operator!=(const Iterator &other) const { return !(*this == other); }
  };

  SparseStorage(size_t capacity = 0,
                const ValueType &defaultValue = ValueType())
      : capacity(capacity), defaultValue(defaultValue) {}

  SparseStorage(const std::vector<ValueType> &values,
                const ValueType &defaultValue = ValueType())
      : capacity(values.capacity()), defaultValue(defaultValue) {
    for (size_t idx = 0; idx < values.capacity(); ++idx) {
      internalStorage[idx] = values[idx];
    }
  }

  void store(size_t idx, const ValueType &value) {
    if (idx < capacity) {
      internalStorage[idx] = value;
    }
  }

  template <typename InputIterator>
  void store(size_t idx, InputIterator iteratorBegin,
             InputIterator iteratorEnd) {
    for (; iteratorBegin != iteratorEnd; ++iteratorBegin, ++idx) {
      store(idx, *iteratorBegin);
    }
  }

  ValueType load(size_t idx) const {
    assert(idx < capacity && idx >= 0);
    return contains(idx) ? internalStorage.at(idx) : defaultValue;
  }

  size_t size() const { return capacity; }

  void resize(size_t newCapacity) {
    assert(newCapacity >= 0);
    // Free to extend
    if (newCapacity >= capacity) {
      capacity = newCapacity;
      return;
    }

    // Truncate unnessecary elements
    auto iterOnNewSize = internalStorage.lower_bound(newCapacity);
    while (iterOnNewSize != internalStorage.end()) {
      iterOnNewSize = internalStorage.erase(iterOnNewSize);
    }

    capacity = newCapacity;
  }

  bool operator==(const SparseStorage<ValueType> &another) const {
    return size() == another.size() && defaultValue == another.defaultValue &&
           internalStorage == another.internalStorage;
  }

  bool operator!=(const SparseStorage<ValueType> &another) const {
    return !(*this == another);
  }

  bool operator<(const SparseStorage &another) const {
    return internalStorage < another.internalStorage;
  }

  bool operator>(const SparseStorage &another) const {
    return internalStorage > another.internalStorage;
  }

  Iterator begin() const { return Iterator(0, this); }
  Iterator end() const { return Iterator(size(), this); }
};

template <typename U>
SparseStorage<unsigned char> sparseBytesFromValue(const U &value) {
  const unsigned char *valueUnsignedCharIterator =
      reinterpret_cast<const unsigned char *>(&value);
  SparseStorage<unsigned char> result(sizeof(value));
  result.store(0, valueUnsignedCharIterator,
               valueUnsignedCharIterator + sizeof(value));
  return result;
}

} // namespace klee

#endif
