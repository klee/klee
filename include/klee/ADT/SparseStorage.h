#ifndef KLEE_SPARSESTORAGE_H
#define KLEE_SPARSESTORAGE_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <unordered_map>
#include <vector>

namespace llvm {
class raw_ostream;
};

namespace klee {

enum class Density {
  Sparse,
  Dense,
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
class SparseStorage {
private:
  std::unordered_map<size_t, ValueType> internalStorage;
  ValueType defaultValue;
  Eq eq;

  bool contains(size_t key) const { return internalStorage.count(key) != 0; }

public:
  SparseStorage(const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {}

  SparseStorage(const std::unordered_map<size_t, ValueType> &internalStorage,
                const ValueType &defaultValue)
      : defaultValue(defaultValue) {
    for (auto &[index, value] : internalStorage) {
      store(index, value);
    }
  }

  SparseStorage(const std::vector<ValueType> &values,
                const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {
    for (size_t idx = 0; idx < values.size(); ++idx) {
      store(idx, values[idx]);
    }
  }

  void store(size_t idx, const ValueType &value) {
    if (eq(value, defaultValue)) {
      internalStorage.erase(idx);
    } else {
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
    auto it = internalStorage.find(idx);
    return it != internalStorage.end() ? it->second : defaultValue;
  }

  size_t sizeOfSetRange() const {
    size_t sizeOfRange = 0;
    for (auto i : internalStorage) {
      sizeOfRange = std::max(i.first, sizeOfRange);
    }
    return internalStorage.empty() ? 0 : sizeOfRange + 1;
  }

  bool operator==(const SparseStorage<ValueType> &another) const {
    return eq(defaultValue, another.defaultValue) && compare(another) == 0;
  }

  bool operator!=(const SparseStorage<ValueType> &another) const {
    return !(*this == another);
  }

  bool operator<(const SparseStorage &another) const {
    return compare(another) == -1;
  }

  bool operator>(const SparseStorage &another) const {
    return compare(another) == 1;
  }

  int compare(const SparseStorage<ValueType> &other) const {
    auto ordered = calculateOrderedStorage();
    auto otherOrdered = other.calculateOrderedStorage();

    if (ordered == otherOrdered) {
      return 0;
    } else {
      return ordered < otherOrdered ? -1 : 1;
    }
  }

  std::map<size_t, ValueType> calculateOrderedStorage() const {
    std::map<size_t, ValueType> ordered;
    for (const auto &i : internalStorage) {
      ordered.insert(i);
    }
    return ordered;
  }

  std::vector<ValueType> getFirstNIndexes(size_t n) const {
    std::vector<ValueType> vectorized(n);
    for (size_t i = 0; i < n; i++) {
      vectorized[i] = load(i);
    }
    return vectorized;
  }

  const std::unordered_map<size_t, ValueType> &storage() const {
    return internalStorage;
  };

  const ValueType &defaultV() const { return defaultValue; };

  void reset() { internalStorage.clear(); }

  void reset(ValueType newDefault) {
    defaultValue = newDefault;
    reset();
  }

  void print(llvm::raw_ostream &os, Density) const;
};

template <typename U>
SparseStorage<unsigned char> sparseBytesFromValue(const U &value) {
  const unsigned char *valueUnsignedCharIterator =
      reinterpret_cast<const unsigned char *>(&value);
  SparseStorage<unsigned char> result;
  result.store(0, valueUnsignedCharIterator,
               valueUnsignedCharIterator + sizeof(value));
  return result;
}

} // namespace klee

#endif
