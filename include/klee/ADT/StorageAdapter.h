#ifndef KLEE_STORAGEADAPTER_H
#define KLEE_STORAGEADAPTER_H

#include "klee/ADT/PersistentHashMap.h"
#include "klee/Support/CompilerWarning.h"

#ifndef IMMER_NO_EXCEPTIONS
#define IMMER_NO_EXCEPTIONS
#endif /* IMMER_NO_EXCEPTIONS */

#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <cstddef>
#include <functional>
#include <unordered_map>

namespace llvm {
class raw_ostream;
};

namespace klee {
enum class StorageIteratorKind { UMap, PersistentUMap, SparseArray };

template <typename ValueType> struct UnorderedMapAdapterIterator {
  using storage_ty = std::unordered_map<size_t, ValueType>;
  using value_ty = std::pair<size_t, ValueType>;
  typename storage_ty::const_iterator it;
  UnorderedMapAdapterIterator(typename storage_ty::const_iterator it)
      : it(it) {}
  UnorderedMapAdapterIterator &operator++() {
    ++it;
    return *this;
  }
  value_ty operator*() { return *it; }
  bool operator!=(const UnorderedMapAdapterIterator &other) const {
    return other.it != it;
  }
};

template <typename ValueType> struct PersistentMapAdapterIterator {
  using storage_ty = PersistentHashMap<size_t, ValueType>;
  using value_ty = std::pair<size_t, ValueType>;
  typename storage_ty::iterator it;
  PersistentMapAdapterIterator(typename storage_ty::iterator it) : it(it) {}
  PersistentMapAdapterIterator &operator++() {
    ++it;
    return *this;
  }
  value_ty operator*() { return *it; }
  bool operator!=(const PersistentMapAdapterIterator &other) const {
    return other.it != it;
  }
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct SparseArrayAdapterIterator {
  using storage_ty = ValueType *;
  using value_ty = std::pair<size_t, ValueType>;
  storage_ty it;
  size_t index;
  size_t size;
  size_t nonDefaultValuesCount;
  ValueType defaultValue;

public:
  SparseArrayAdapterIterator(storage_ty _it, size_t _index, size_t _size,
                             size_t _nonDefaultValuesCount,
                             const ValueType &_defaultValue)
      : it(_it), index(_index), size(_size),
        nonDefaultValuesCount(_nonDefaultValuesCount),
        defaultValue(_defaultValue) {
    if (nonDefaultValuesCount < size) {
      while (index < size && Eq()(*it, defaultValue)) {
        ++it;
        ++index;
      }
    }
  }
  SparseArrayAdapterIterator &operator++() {
    ++it;
    ++index;
    if (nonDefaultValuesCount < size) {
      while (index < size && Eq()(*it, defaultValue)) {
        ++it;
        ++index;
      }
    }
    return *this;
  }
  value_ty operator*() { return {index, *it}; }
  bool operator!=(const SparseArrayAdapterIterator &other) const {
    return other.it != it || other.index != index;
  }
  SparseArrayAdapterIterator(storage_ty it) : it(it) {}
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
union StorageIterator {
  UnorderedMapAdapterIterator<ValueType> umaIt;
  PersistentMapAdapterIterator<ValueType> pumaIt;
  SparseArrayAdapterIterator<ValueType, Eq> saaIt;
  ~StorageIterator() {}
  StorageIterator(const UnorderedMapAdapterIterator<ValueType> &other)
      : umaIt(other) {}
  StorageIterator(const PersistentMapAdapterIterator<ValueType> &other)
      : pumaIt(other) {}
  StorageIterator(const SparseArrayAdapterIterator<ValueType, Eq> &other)
      : saaIt(other) {}
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct StorageAdapter {
  using value_ty = std::pair<size_t, ValueType>;
  virtual ~StorageAdapter() = default;
  class iterator {
    StorageIteratorKind kind;
    StorageIterator<ValueType, Eq> impl;

  public:
    iterator(const UnorderedMapAdapterIterator<ValueType> &impl)
        : kind(StorageIteratorKind::UMap), impl(impl) {}
    iterator(const PersistentMapAdapterIterator<ValueType> &impl)
        : kind(StorageIteratorKind::PersistentUMap), impl(impl) {}
    iterator(const SparseArrayAdapterIterator<ValueType, Eq> &impl)
        : kind(StorageIteratorKind::SparseArray), impl(impl) {}
    iterator(iterator const &right) : kind(right.kind) {
      switch (kind) {
      case klee::StorageIteratorKind::UMap: {
        impl.umaIt = right.impl.umaIt;
        break;
      }
      case klee::StorageIteratorKind::PersistentUMap: {
        impl.pumaIt = right.impl.pumaIt;
        break;
      }
      case klee::StorageIteratorKind::SparseArray: {
        impl.saaIt = right.impl.saaIt;
        break;
      }
      default:
        assert(0 && "unhandled iterator kind");
        unreachable();
      }
    }
    ~iterator() {
      switch (kind) {
      case klee::StorageIteratorKind::UMap: {
        impl.umaIt.~UnorderedMapAdapterIterator();
        break;
      }
      case klee::StorageIteratorKind::PersistentUMap: {
        impl.pumaIt.~PersistentMapAdapterIterator();
        break;
      }
      case klee::StorageIteratorKind::SparseArray: {
        impl.saaIt.~SparseArrayAdapterIterator();
        break;
      }
      default:
        assert(0 && "unhandled iterator kind");
        unreachable();
      }
    }

    // forward operators to virtual calls through impl.
    iterator &operator++() {
      switch (kind) {
      case klee::StorageIteratorKind::UMap: {
        ++impl.umaIt;
        break;
      }
      case klee::StorageIteratorKind::PersistentUMap: {
        ++impl.pumaIt;
        break;
      }
      case klee::StorageIteratorKind::SparseArray: {
        ++impl.saaIt;
        break;
      }
      default:
        assert(0 && "unhandled iterator kind");
        unreachable();
      }
      return *this;
    }
    value_ty operator*() {
      switch (kind) {
      case klee::StorageIteratorKind::UMap: {
        return *impl.umaIt;
      }
      case klee::StorageIteratorKind::PersistentUMap: {
        return *impl.pumaIt;
      }
      case klee::StorageIteratorKind::SparseArray: {
        return *impl.saaIt;
      }
      default:
        assert(0 && "unhandled iterator kind");
        unreachable();
      }
    }
    bool operator!=(const iterator &other) const {
      switch (kind) {
      case klee::StorageIteratorKind::UMap: {
        return impl.umaIt != other.impl.umaIt;
      }
      case klee::StorageIteratorKind::PersistentUMap: {
        return impl.pumaIt != other.impl.pumaIt;
      }
      case klee::StorageIteratorKind::SparseArray: {
        return impl.saaIt != other.impl.saaIt;
      }
      default:
        assert(0 && "unhandled iterator kind");
        unreachable();
      }
    }
  };

  virtual bool contains(size_t key) const = 0;
  virtual iterator begin() const = 0;
  virtual iterator end() const = 0;
  virtual const ValueType *lookup(size_t key) const = 0;
  virtual bool empty() const = 0;
  virtual void set(size_t key, const ValueType &value) = 0;
  virtual void remove(size_t key) = 0;
  virtual const ValueType &at(size_t key) const = 0;
  virtual void clear() = 0;
  virtual size_t size() const = 0;
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct UnorderedMapAdapder : public StorageAdapter<ValueType, Eq> {
public:
  using storage_ty = std::unordered_map<size_t, ValueType>;
  using base_ty = StorageAdapter<ValueType, Eq>;
  using iterator = typename base_ty::iterator;
  struct constructor {
    UnorderedMapAdapder<ValueType, Eq> operator()(const ValueType &) const {
      return UnorderedMapAdapder<ValueType, Eq>();
    }
  };

private:
  storage_ty storage;

public:
  UnorderedMapAdapder() = default;
  bool contains(size_t key) const override { return storage.count(key) != 0; }
  iterator begin() const override {
    return iterator(UnorderedMapAdapterIterator<ValueType>(storage.begin()));
  }
  iterator end() const override {
    return iterator(UnorderedMapAdapterIterator<ValueType>(storage.end()));
  }
  const ValueType *lookup(size_t key) const override {
    auto it = storage.find(key);
    if (it != storage.end()) {
      return &it->second;
    }
    return nullptr;
  }
  bool empty() const override { return storage.empty(); }
  void set(size_t key, const ValueType &value) override {
    storage[key] = value;
  }
  void remove(size_t key) override { storage.erase(key); }
  const ValueType &at(size_t key) const override { return storage.at(key); }
  void clear() override { storage.clear(); }
  size_t size() const override { return storage.size(); }
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct PersistenUnorderedMapAdapder : public StorageAdapter<ValueType, Eq> {
  using storage_ty = PersistentHashMap<size_t, ValueType>;
  using base_ty = StorageAdapter<ValueType, Eq>;
  using iterator = typename base_ty::iterator;
  struct constructor {
    PersistenUnorderedMapAdapder<ValueType, Eq>
    operator()(const ValueType &) const {
      return PersistenUnorderedMapAdapder<ValueType, Eq>();
    }
  };

private:
  storage_ty storage;

public:
  PersistenUnorderedMapAdapder() = default;

  bool contains(size_t key) const override { return storage.count(key) != 0; }
  iterator begin() const override {
    return iterator(PersistentMapAdapterIterator<ValueType>(storage.begin()));
  }
  iterator end() const override {
    return iterator(PersistentMapAdapterIterator<ValueType>(storage.end()));
  }
  const ValueType *lookup(size_t key) const override {
    return storage.lookup(key);
  }
  bool empty() const override { return storage.empty(); }
  void set(size_t key, const ValueType &value) override {
    storage.replace({key, value});
  }
  void remove(size_t key) override { storage.remove(key); }
  const ValueType &at(size_t key) const override { return storage.at(key); }
  void clear() override { storage.clear(); }
  size_t size() const override { return storage.size(); }
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct SparseArrayAdapter : public StorageAdapter<ValueType, Eq> {
  using storage_ty = ValueType *;
  using base_ty = StorageAdapter<ValueType, Eq>;
  using iterator = typename base_ty::iterator;
  struct constructor {
    size_t storageSize;
    constructor(size_t storageSize) : storageSize(storageSize) {}
    SparseArrayAdapter<ValueType, Eq>
    operator()(const ValueType &defaultValue) const {
      return SparseArrayAdapter<ValueType, Eq>(defaultValue, storageSize);
    }
  };

private:
  storage_ty storage;
  size_t storageSize;
  ValueType defaultValue;
  size_t nonDefaultValuesCount;

public:
  SparseArrayAdapter(const ValueType &defaultValue, size_t storageSize)
      : storageSize(storageSize), defaultValue(defaultValue),
        nonDefaultValuesCount(0) {
    storage = new ValueType[storageSize];
    clear();
  }
  SparseArrayAdapter(const SparseArrayAdapter<ValueType, Eq> &pa)
      : storageSize(pa.storageSize), defaultValue(pa.defaultValue) {
    storage = new ValueType[storageSize];
    clear();
    for (const auto &[key, val] : pa) {
      set(key, val);
    }
  }
  SparseArrayAdapter &operator=(const SparseArrayAdapter<ValueType, Eq> &pa) {
    storageSize = pa.storageSize;
    defaultValue = pa.defaultValue;
    delete[] storage;
    storage = new ValueType[storageSize];
    clear();
    for (const auto &[key, val] : pa) {
      set(key, val);
    }
    return *this;
  }
  ~SparseArrayAdapter() { delete[] storage; }
  bool contains(size_t key) const override { return storage[key] != 0; }
  iterator begin() const override {
    return iterator(SparseArrayAdapterIterator<ValueType, Eq>(
        storage, 0, storageSize, nonDefaultValuesCount, defaultValue));
  }
  iterator end() const override {
    return iterator(SparseArrayAdapterIterator<ValueType, Eq>(
        storage + storageSize, storageSize, storageSize, nonDefaultValuesCount,
        defaultValue));
  }
  const ValueType *lookup(size_t key) const override {
    if (key >= storageSize) {
      return nullptr;
    }
    auto val = storage[key];
    return Eq()(val, defaultValue) ? nullptr : &storage[key];
  }
  bool empty() const override { return nonDefaultValuesCount == 0; }
  void set(size_t key, const ValueType &value) override {
    bool wasDefault = Eq()(storage[key], defaultValue);
    bool newDefault = Eq()(value, defaultValue);
    if (wasDefault && !newDefault) {
      ++nonDefaultValuesCount;
    }
    if (!wasDefault && newDefault) {
      --nonDefaultValuesCount;
    }
    storage[key] = value;
  }
  void remove(size_t key) override {
    if (!Eq()(storage[key], defaultValue)) {
      --nonDefaultValuesCount;
      storage[key] = defaultValue;
    }
  }
  const ValueType &at(size_t key) const override { return storage[key]; }
  void clear() override {
    for (size_t i = 0; i < storageSize; ++i) {
      storage[i] = defaultValue;
    }
    nonDefaultValuesCount = 0;
  }
  size_t size() const override { return nonDefaultValuesCount; }
};

} // namespace klee

#endif
