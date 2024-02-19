#ifndef KLEE_SPARSESTORAGE_H
#define KLEE_SPARSESTORAGE_H

#include "klee/ADT/PersistentHashMap.h"

#ifndef IMMER_NO_EXCEPTIONS
#define IMMER_NO_EXCEPTIONS
#endif /* IMMER_NO_EXCEPTIONS */

#include <immer/vector.hpp>

#include <cassert>
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

enum class StorageIteratorKind { UMap, PersistentUMap, SparseArray };
enum class FixedSizeStorageIteratorKind { Array, Vector, PersistentVector };

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct StorageAdapter {
  using value_ty = std::pair<size_t, ValueType>;
  class inner_iterator {
  public:
    virtual StorageIteratorKind getKind() const = 0;
    virtual inner_iterator &operator++() = 0;
    virtual inner_iterator *clone() = 0;
    virtual value_ty operator*() = 0;
    virtual bool operator!=(const inner_iterator &other) const = 0;
    static bool classof(const StorageAdapter<ValueType, Eq> *SE) {
      return true;
    }
    virtual ~inner_iterator() {}
  };

  class iterator {
    inner_iterator *impl;

  public:
    iterator(inner_iterator *impl) : impl(impl) {}
    iterator(iterator const &right) : impl(right.impl->clone()) {}
    ~iterator() { delete impl; }
    iterator &operator=(iterator const &right) {
      delete impl;
      impl = right.impl->clone();
      return *this;
    }

    // forward operators to virtual calls through impl.
    iterator &operator++() {
      ++(*impl);
      return *this;
    }
    value_ty operator*() { return *(*impl); }
    bool operator!=(const iterator &other) const {
      return *impl != *other.impl;
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

template <typename ValueType> struct FixedSizeStorageAdapter {
  using value_ty = ValueType;
  class inner_iterator {
  public:
    virtual FixedSizeStorageIteratorKind getKind() const = 0;
    virtual inner_iterator &operator++() = 0;
    virtual inner_iterator *clone() = 0;
    virtual value_ty operator*() = 0;
    virtual bool operator!=(const inner_iterator &other) const = 0;
    static bool classof(const FixedSizeStorageAdapter<ValueType> *SE) {
      return true;
    }
    virtual ~inner_iterator() {}
  };

  class iterator {
    inner_iterator *impl;

  public:
    iterator(inner_iterator *impl) : impl(impl) {}
    iterator(iterator const &right) : impl(right.impl->clone()) {}
    ~iterator() { delete impl; }
    iterator &operator=(iterator const &right) {
      delete impl;
      impl = right.impl->clone();
      return *this;
    }

    // forward operators to virtual calls through impl.
    iterator &operator++() {
      ++(*impl);
      return *this;
    }
    value_ty operator*() { return *(*impl); }
    bool operator!=(const iterator &other) const {
      return *impl != *other.impl;
    }
  };

  virtual ~FixedSizeStorageAdapter() {}
  virtual iterator begin() const = 0;
  virtual iterator end() const = 0;
  virtual bool empty() const = 0;
  virtual void set(size_t key, const ValueType &value) = 0;
  virtual const value_ty &at(size_t key) const = 0;
  virtual size_t size() const = 0;
  virtual FixedSizeStorageAdapter<ValueType> *clone() const = 0;
  const value_ty &operator[](const size_t &index) const { return at(index); }
};

template <typename ValueType>
struct VectorAdapter : public FixedSizeStorageAdapter<ValueType> {
public:
  using storage_ty = std::vector<ValueType>;
  using base_ty = FixedSizeStorageAdapter<ValueType>;
  using iterator = typename base_ty::iterator;
  using inner_iterator = typename base_ty::inner_iterator;
  class vector_iterator : public inner_iterator {
    typename storage_ty::const_iterator it;

  public:
    vector_iterator(const typename storage_ty::const_iterator &it) : it(it) {}
    FixedSizeStorageIteratorKind getKind() const override {
      return FixedSizeStorageIteratorKind::Vector;
    }
    static bool classof(const FixedSizeStorageAdapter<ValueType> *SA) {
      return SA->getKind() == FixedSizeStorageIteratorKind::Vector;
    }
    static bool classof(const VectorAdapter<ValueType> *) { return true; }
    inner_iterator &operator++() override {
      ++it;
      return *this;
    }
    inner_iterator *clone() override { return new vector_iterator(it); }
    typename base_ty::value_ty operator*() override { return *it; }
    bool operator!=(const inner_iterator &other) const override {
      if (other.getKind() != getKind()) {
        return true;
      }
      const vector_iterator &el = static_cast<const vector_iterator &>(other);
      return el.it != it;
    }
  };

  VectorAdapter(size_t storageSize) { storage = storage_ty(storageSize); }
  VectorAdapter(const VectorAdapter<ValueType> &va) : storage(va.storage) {}
  storage_ty storage;
  iterator begin() const override {
    return iterator(new vector_iterator(storage.begin()));
  }
  iterator end() const override {
    return iterator(new vector_iterator(storage.end()));
  }
  bool empty() const override { return storage.empty(); }
  void set(size_t key, const ValueType &value) override {
    storage[key] = value;
  }
  const ValueType &at(size_t key) const override { return storage.at(key); }
  size_t size() const override { return storage.size(); }
  FixedSizeStorageAdapter<ValueType> *clone() const override {
    return new VectorAdapter<ValueType>(*this);
  }
};

template <typename ValueType>
struct PersistentVectorAdapter : public FixedSizeStorageAdapter<ValueType> {
public:
  using storage_ty = immer::vector<ValueType>;
  using base_ty = FixedSizeStorageAdapter<ValueType>;
  using iterator = typename base_ty::iterator;
  using inner_iterator = typename base_ty::inner_iterator;
  class vector_iterator : public inner_iterator {
    typename storage_ty::iterator it;

  public:
    vector_iterator(const typename storage_ty::const_iterator &it) : it(it) {}
    FixedSizeStorageIteratorKind getKind() const override {
      return FixedSizeStorageIteratorKind::Vector;
    }
    static bool classof(const FixedSizeStorageAdapter<ValueType> *SA) {
      return SA->getKind() == FixedSizeStorageIteratorKind::Vector;
    }
    static bool classof(const PersistentVectorAdapter<ValueType> *) {
      return true;
    }
    inner_iterator &operator++() override {
      ++it;
      return *this;
    }
    inner_iterator *clone() override { return new vector_iterator(it); }
    typename base_ty::value_ty operator*() override { return *it; }
    bool operator!=(const inner_iterator &other) const override {
      if (other.getKind() != getKind()) {
        return true;
      }
      const vector_iterator &el = static_cast<const vector_iterator &>(other);
      return el.it != it;
    }
  };
  PersistentVectorAdapter(size_t storageSize) {
    storage = storage_ty(storageSize);
  }
  PersistentVectorAdapter(const PersistentVectorAdapter<ValueType> &va)
      : storage(va.storage) {}
  storage_ty storage;
  iterator begin() const override {
    return iterator(new vector_iterator(storage.begin()));
  }
  iterator end() const override {
    return iterator(new vector_iterator(storage.end()));
  }
  bool empty() const override { return storage.empty(); }
  void set(size_t key, const ValueType &value) override {
    storage = storage.set(key, value);
  }
  const ValueType &at(size_t key) const override { return storage.at(key); }
  size_t size() const override { return storage.size(); }
  FixedSizeStorageAdapter<ValueType> *clone() const override {
    return new PersistentVectorAdapter<ValueType>(*this);
  }
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct UnorderedMapAdapder : public StorageAdapter<ValueType, Eq> {
public:
  using storage_ty = std::unordered_map<size_t, ValueType>;
  using base_ty = StorageAdapter<ValueType, Eq>;
  using iterator = typename base_ty::iterator;
  using inner_iterator = typename base_ty::inner_iterator;
  struct allocator {
    UnorderedMapAdapder<ValueType, Eq> operator()(const ValueType &) const {
      return UnorderedMapAdapder<ValueType, Eq>();
    }
  };
  class umap_iterator : public inner_iterator {
    typename storage_ty::const_iterator it;

  public:
    umap_iterator(const typename storage_ty::const_iterator &it) : it(it) {}
    StorageIteratorKind getKind() const override {
      return StorageIteratorKind::UMap;
    }
    static bool classof(const StorageAdapter<ValueType, Eq> *SA) {
      return SA->getKind() == StorageIteratorKind::UMap;
    }
    static bool classof(const UnorderedMapAdapder<ValueType, Eq> *) {
      return true;
    }
    inner_iterator &operator++() override {
      ++it;
      return *this;
    }
    inner_iterator *clone() override { return new umap_iterator(it); }
    typename base_ty::value_ty operator*() override { return *it; }
    bool operator!=(const inner_iterator &other) const override {
      if (other.getKind() != getKind()) {
        return true;
      }
      const umap_iterator &el = static_cast<const umap_iterator &>(other);
      return el.it != it;
    }
  };
  storage_ty storage;
  bool contains(size_t key) const override { return storage.count(key) != 0; }
  iterator begin() const override {
    return iterator(new umap_iterator(storage.begin()));
  }
  iterator end() const override {
    return iterator(new umap_iterator(storage.end()));
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
  using inner_iterator = typename base_ty::inner_iterator;
  struct allocator {
    PersistenUnorderedMapAdapder<ValueType, Eq>
    operator()(const ValueType &) const {
      return PersistenUnorderedMapAdapder<ValueType, Eq>();
    }
  };
  class persistent_umap_iterator : public inner_iterator {
    typename storage_ty::iterator it;

  public:
    persistent_umap_iterator(const typename storage_ty::iterator &it)
        : it(it) {}
    StorageIteratorKind getKind() const override {
      return StorageIteratorKind::PersistentUMap;
    }
    static bool classof(const StorageAdapter<ValueType, Eq> *SA) {
      return SA->getKind() == StorageIteratorKind::PersistentUMap;
    }
    static bool classof(const PersistenUnorderedMapAdapder<ValueType, Eq> *) {
      return true;
    }
    inner_iterator &operator++() override {
      ++it;
      return *this;
    }
    inner_iterator *clone() override {
      return new persistent_umap_iterator(it);
    }
    typename base_ty::value_ty operator*() override { return *it; }
    bool operator!=(const inner_iterator &other) const override {
      if (other.getKind() != getKind()) {
        return true;
      }
      const persistent_umap_iterator &el =
          static_cast<const persistent_umap_iterator &>(other);
      return el.it != it;
    }
  };
  storage_ty storage;
  bool contains(size_t key) const override { return storage.count(key) != 0; }
  iterator begin() const override {
    return new persistent_umap_iterator(storage.begin());
  }
  iterator end() const override {
    return new persistent_umap_iterator(storage.end());
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

template <typename ValueType>
struct ArrayAdapter : public FixedSizeStorageAdapter<ValueType> {
  using storage_ty = ValueType *;
  using base_ty = FixedSizeStorageAdapter<ValueType>;
  using iterator = typename base_ty::iterator;
  using inner_iterator = typename base_ty::inner_iterator;

  class array_iterator : public inner_iterator {
    storage_ty it;

  public:
    array_iterator(storage_ty _it) : it(_it) {}
    FixedSizeStorageIteratorKind getKind() const override {
      return FixedSizeStorageIteratorKind::Array;
    }
    static bool classof(const FixedSizeStorageAdapter<ValueType> *SA) {
      return SA->getKind() == FixedSizeStorageIteratorKind::Array;
    }
    static bool classof(const ArrayAdapter<ValueType> *) { return true; }
    inner_iterator &operator++() override {
      ++it;
      return *this;
    }
    inner_iterator *clone() override { return new array_iterator(it); }
    typename base_ty::value_ty operator*() override { return *it; }
    bool operator!=(const inner_iterator &other) const override {
      if (other.getKind() != getKind()) {
        return true;
      }
      const array_iterator &el = static_cast<const array_iterator &>(other);
      return el.it != it;
    }
  };

public:
  storage_ty storage;
  size_t storageSize;
  ArrayAdapter(size_t storageSize) : storageSize(storageSize) {
    storage = new ValueType[storageSize];
    clear();
  }
  ArrayAdapter(const ArrayAdapter<ValueType> &pa)
      : storageSize(pa.storageSize) {
    storage = new ValueType[storageSize];
    for (size_t i = 0; i < storageSize; ++i) {
      set(i, pa.at(i));
    }
  }
  ArrayAdapter &operator=(const ArrayAdapter<ValueType> &pa) {
    storageSize = pa.storageSize;
    delete[] storage;
    storage = new ValueType[storageSize];
    for (size_t i = 0; i < storageSize; ++i) {
      set(i, pa.at(i));
    }
    return *this;
  }
  ~ArrayAdapter() override { delete[] storage; }
  iterator begin() const override { return new array_iterator(storage); }
  iterator end() const override {
    return new array_iterator(storage + storageSize);
  }
  bool empty() const override { return storageSize == 0; }
  void set(size_t key, const ValueType &value) override {
    storage[key] = value;
  }
  const ValueType &at(size_t key) const override { return storage[key]; }
  void clear() {
    for (size_t i = 0; i < storageSize; ++i) {
      set(i, ValueType());
    }
  }
  size_t size() const override { return storageSize; }
  FixedSizeStorageAdapter<ValueType> *clone() const override {
    return new ArrayAdapter<ValueType>(*this);
  }
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct SparseArrayAdapter : public StorageAdapter<ValueType, Eq> {
  using storage_ty = ValueType *;
  using base_ty = StorageAdapter<ValueType, Eq>;
  using iterator = typename base_ty::iterator;
  using inner_iterator = typename base_ty::inner_iterator;
  struct allocator {
    size_t storageSize;
    allocator(size_t storageSize) : storageSize(storageSize) {}
    SparseArrayAdapter<ValueType, Eq>
    operator()(const ValueType &defaultValue) const {
      return SparseArrayAdapter<ValueType, Eq>(defaultValue, storageSize);
    }
  };

  Eq eq;
  class persistent_array_iterator : public inner_iterator {
    storage_ty it;
    size_t index;
    size_t size;
    size_t nonDefaultValuesCount;
    Eq eq;
    ValueType defaultValue;

  public:
    persistent_array_iterator(storage_ty _it, size_t _index, size_t _size,
                              size_t _nonDefaultValuesCount,
                              const ValueType &_defaultValue)
        : it(_it), index(_index), size(_size),
          nonDefaultValuesCount(_nonDefaultValuesCount),
          defaultValue(_defaultValue) {
      if (nonDefaultValuesCount < size) {
        while (index < size && eq(*it, defaultValue)) {
          ++it;
          ++index;
        }
      }
    }
    StorageIteratorKind getKind() const override {
      return StorageIteratorKind::SparseArray;
    }
    static bool classof(const StorageAdapter<ValueType, Eq> *SA) {
      return SA->getKind() == StorageIteratorKind::SparseArray;
    }
    static bool classof(const SparseArrayAdapter<ValueType, Eq> *) {
      return true;
    }
    inner_iterator &operator++() override {
      ++it;
      ++index;
      if (nonDefaultValuesCount < size) {
        while (index < size && eq(*it, defaultValue)) {
          ++it;
          ++index;
        }
      }
      return *this;
    }
    inner_iterator *clone() override {
      return new persistent_array_iterator(it, index, size,
                                           nonDefaultValuesCount, defaultValue);
    }
    typename base_ty::value_ty operator*() override { return {index, *it}; }
    bool operator!=(const inner_iterator &other) const override {
      if (other.getKind() != getKind()) {
        return true;
      }
      const persistent_array_iterator &el =
          static_cast<const persistent_array_iterator &>(other);
      return el.it != it || el.index != index;
    }
  };

public:
  storage_ty storage;
  size_t storageSize;
  ValueType defaultValue;
  size_t nonDefaultValuesCount;
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
    return new persistent_array_iterator(storage, 0, storageSize,
                                         nonDefaultValuesCount, defaultValue);
  }
  iterator end() const override {
    return new persistent_array_iterator(storage + storageSize, storageSize,
                                         storageSize, nonDefaultValuesCount,
                                         defaultValue);
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
    bool wasDefault = eq(storage[key], defaultValue);
    bool newDefault = eq(value, defaultValue);
    if (wasDefault && !newDefault) {
      ++nonDefaultValuesCount;
    }
    if (!wasDefault && newDefault) {
      --nonDefaultValuesCount;
    }
    storage[key] = value;
  }
  void remove(size_t key) override {
    if (!eq(storage[key], defaultValue)) {
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

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
class Storage {
protected:
  ValueType defaultValue;
  Eq eq;

  Storage(const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {}

public:
  virtual ~Storage() {}

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

  bool operator==(const Storage<ValueType, Eq> &another) const {
    return eq(defaultValue, another.defaultValue) && compare(another) == 0;
  }

  bool operator!=(const Storage<ValueType, Eq> &another) const {
    return !(*this == another);
  }

  bool operator<(const Storage &another) const {
    return compare(another) == -1;
  }

  bool operator>(const Storage &another) const { return compare(another) == 1; }

  int compare(const Storage<ValueType, Eq> &other) const {
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

  virtual Storage<ValueType, Eq> *clone() const = 0;
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>,
          typename InternalStorageAdapter = UnorderedMapAdapder<ValueType, Eq>,
          typename Allocator = typename InternalStorageAdapter::allocator>
class SparseStorage : public Storage<ValueType, Eq> {
private:
  Allocator alloc;
  InternalStorageAdapter internalStorage;

  bool contains(size_t key) const { return internalStorage.containts(key); }

public:
  // SparseStorage(const ValueType &defaultValue = ValueType())
  //     : Storage<ValueType, Eq>(defaultValue) {
  //   static_assert(
  //       std::is_base_of<StorageAdapter<ValueType, Eq>,
  //                       InternalStorageAdapter>::value,
  //       "type parameter of this class must derive from StorageAdapter");
  // }

  SparseStorage(const ValueType &defaultValue = ValueType(),
                const Allocator &alloc = Allocator())
      : Storage<ValueType, Eq>(defaultValue), alloc(alloc),
        internalStorage(alloc(defaultValue)) {
    static_assert(
        std::is_base_of<StorageAdapter<ValueType, Eq>,
                        InternalStorageAdapter>::value,
        "type parameter of this class must derive from StorageAdapter");
  }

  SparseStorage(const std::unordered_map<size_t, ValueType> &internalStorage,
                const ValueType &defaultValue = ValueType(),
                const Allocator &alloc = Allocator())
      : SparseStorage(defaultValue, alloc) {
    for (auto &[index, value] : internalStorage) {
      store(index, value);
    }
  }

  SparseStorage(const std::vector<ValueType> &values,
                const ValueType &defaultValue = ValueType(),
                const Allocator &alloc = Allocator())
      : SparseStorage(defaultValue, alloc) {
    for (size_t idx = 0; idx < values.size(); ++idx) {
      store(idx, values[idx]);
    }
  }

  void store(size_t idx, const ValueType &value) override {
    if (Storage<ValueType, Eq>::eq(value,
                                   Storage<ValueType, Eq>::defaultValue)) {
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
    return it ? *it : Storage<ValueType, Eq>::defaultValue;
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
    internalStorage = alloc(Storage<ValueType, Eq>::defaultValue);
  }

  void reset(ValueType newDefault) {
    Storage<ValueType, Eq>::defaultValue = newDefault;
    reset();
  }

  void print(llvm::raw_ostream &os, Density) const;
  Storage<ValueType, Eq> *clone() const override {
    return new SparseStorage<ValueType, Eq, InternalStorageAdapter>(*this);
  }
};

} // namespace klee

#endif
