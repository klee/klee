#ifndef KLEE_SPARSESTORAGE_H
#define KLEE_SPARSESTORAGE_H

#include "klee/ADT/StorageAdapter.h"

#ifndef IMMER_NO_EXCEPTIONS
#define IMMER_NO_EXCEPTIONS
#endif /* IMMER_NO_EXCEPTIONS */

#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <llvm/Support/raw_ostream.h>

#include <cstddef>
#include <functional>
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
protected:
  ValueType defaultValue;

  SparseStorage(const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {}

public:
  virtual ~SparseStorage() = default;

  virtual void store(size_t idx, const ValueType &value) = 0;

  template <typename InputIterator>
  void store(size_t idx, InputIterator iteratorBegin,
             InputIterator iteratorEnd) {
    for (; iteratorBegin != iteratorEnd; ++iteratorBegin, ++idx) {
      store(idx, *iteratorBegin);
    }
  }

  virtual ValueType load(size_t idx) const = 0;

  virtual size_t sizeOfSetRange() const = 0;

  bool operator==(const SparseStorage<ValueType, Eq> &another) const {
    return Eq()(defaultValue, another.defaultValue) && compare(another) == 0;
  }

  bool operator!=(const SparseStorage<ValueType, Eq> &another) const {
    return !(*this == another);
  }

  bool operator<(const SparseStorage &another) const {
    return compare(another) == -1;
  }

  bool operator>(const SparseStorage &another) const {
    return compare(another) == 1;
  }

  int compare(const SparseStorage<ValueType, Eq> &other) const {
    auto ordered = calculateOrderedStorage();
    auto otherOrdered = other.calculateOrderedStorage();

    if (ordered == otherOrdered) {
      return 0;
    } else {
      return ordered < otherOrdered ? -1 : 1;
    }
  }

  virtual std::map<size_t, ValueType> calculateOrderedStorage() const = 0;

  virtual std::vector<ValueType> getFirstNIndexes(size_t n) const = 0;

  virtual const StorageAdapter<ValueType, Eq> &storage() const = 0;

  const ValueType &defaultV() const { return defaultValue; };

  virtual void reset() = 0;

  void reset(ValueType newDefault) {
    defaultValue = newDefault;
    reset();
  }

  void print(llvm::raw_ostream &os, Density) const;
  void dump() const { print(llvm::errs(), Density::Sparse); }

  virtual SparseStorage<ValueType, Eq> *clone() const = 0;
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>,
          typename InternalStorageAdapter = UnorderedMapAdapder<ValueType, Eq>,
          typename Constructor = typename InternalStorageAdapter::constructor>
class SparseStorageImpl : public SparseStorage<ValueType, Eq> {
private:
  Constructor alloc;
  InternalStorageAdapter internalStorage;

public:
  SparseStorageImpl(const ValueType &defaultValue = ValueType(),
                    const Constructor &alloc = Constructor())
      : SparseStorage<ValueType, Eq>(defaultValue), alloc(alloc),
        internalStorage(alloc(defaultValue)) {
    static_assert(
        std::is_base_of<StorageAdapter<ValueType, Eq>,
                        InternalStorageAdapter>::value,
        "type parameter of this class must derive from StorageAdapter");
  }

  SparseStorageImpl(
      const std::unordered_map<size_t, ValueType> &internalStorage,
      const ValueType &defaultValue = ValueType(),
      const Constructor &alloc = Constructor())
      : SparseStorageImpl(defaultValue, alloc) {
    for (const auto &[index, value] : internalStorage) {
      store(index, value);
    }
  }

  SparseStorageImpl(const std::vector<ValueType> &values,
                    const ValueType &defaultValue = ValueType(),
                    const Constructor &alloc = Constructor())
      : SparseStorageImpl(defaultValue, alloc) {
    for (size_t idx = 0; idx < values.size(); ++idx) {
      store(idx, values[idx]);
    }
  }

  void store(size_t idx, const ValueType &value) override {
    if (Eq()(value, SparseStorage<ValueType, Eq>::defaultValue)) {
      internalStorage.remove(idx);
    } else {
      internalStorage.set(idx, value);
    }
  }

  template <typename InputIterator>
  void store(size_t idx, InputIterator iteratorBegin,
             InputIterator iteratorEnd) {
    for (; iteratorBegin != iteratorEnd; ++iteratorBegin, ++idx) {
      store(idx, *iteratorBegin);
    }
  }

  ValueType load(size_t idx) const override {
    auto it = internalStorage.lookup(idx);
    return it ? *it : SparseStorage<ValueType, Eq>::defaultValue;
  }

  size_t sizeOfSetRange() const override {
    size_t sizeOfRange = 0;
    for (auto i : internalStorage) {
      sizeOfRange = std::max(i.first, sizeOfRange);
    }
    return internalStorage.empty() ? 0 : sizeOfRange + 1;
  }

  std::map<size_t, ValueType> calculateOrderedStorage() const override {
    std::map<size_t, ValueType> ordered;
    for (const auto &i : internalStorage) {
      ordered.insert(i);
    }
    return ordered;
  }

  std::vector<ValueType> getFirstNIndexes(size_t n) const override {
    std::vector<ValueType> vectorized(n);
    for (size_t i = 0; i < n; i++) {
      vectorized[i] = load(i);
    }
    return vectorized;
  }

  const InternalStorageAdapter &storage() const override {
    return internalStorage;
  };

  void reset() override {
    internalStorage = alloc(SparseStorage<ValueType, Eq>::defaultValue);
  }

  void reset(ValueType newDefault) {
    SparseStorage<ValueType, Eq>::defaultValue = newDefault;
    reset();
  }

  void print(llvm::raw_ostream &os, Density) const;
  void dump() const { print(llvm::errs(), Density::Sparse); }
  SparseStorage<ValueType, Eq> *clone() const override {
    return new SparseStorageImpl<ValueType, Eq, InternalStorageAdapter>(*this);
  }
};

} // namespace klee

#endif
