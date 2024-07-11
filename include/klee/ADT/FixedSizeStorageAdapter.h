#ifndef KLEE_FIXEDSIZESTORAGEADAPTER_H
#define KLEE_FIXEDSIZESTORAGEADAPTER_H

#include "klee/Support/CompilerWarning.h"

#ifndef IMMER_NO_EXCEPTIONS
#define IMMER_NO_EXCEPTIONS
#endif /* IMMER_NO_EXCEPTIONS */

#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <cstddef>
#include <vector>

namespace llvm {
class raw_ostream;
};

namespace klee {

enum class FixedSizeStorageIteratorKind { Array, Vector, PersistentVector };

template <typename ValueType> struct VectorAdapterIterator {
  using storage_ty = std::vector<ValueType>;
  using value_ty = ValueType;
  typename storage_ty::const_iterator it;
  VectorAdapterIterator(typename storage_ty::const_iterator it) : it(it) {}
  VectorAdapterIterator &operator++() {
    ++it;
    return *this;
  }
  value_ty operator*() { return *it; }
  bool operator!=(const VectorAdapterIterator &other) const {
    return other.it != it;
  }
};

template <typename ValueType> struct PersistentVectorAdapterIterator {
  using storage_ty = immer::vector_transient<ValueType>;
  using value_ty = ValueType;
  typename storage_ty::iterator it;
  PersistentVectorAdapterIterator(typename storage_ty::iterator it) : it(it) {}
  PersistentVectorAdapterIterator &operator++() {
    ++it;
    return *this;
  }
  value_ty operator*() { return *it; }
  bool operator!=(const PersistentVectorAdapterIterator &other) const {
    return other.it != it;
  }
};

template <typename ValueType> struct ArrayAdapterIterator {
  using storage_ty = ValueType *;
  using value_ty = ValueType;
  storage_ty it;

public:
  ArrayAdapterIterator(storage_ty it) : it(it) {}
  ArrayAdapterIterator &operator++() {
    ++it;
    return *this;
  }
  value_ty operator*() { return *it; }
  bool operator!=(const ArrayAdapterIterator &other) const {
    return other.it != it;
  }
};

template <typename ValueType> union FixedSizeStorageIterator {
  VectorAdapterIterator<ValueType> vaIt;
  PersistentVectorAdapterIterator<ValueType> pvaIt;
  ArrayAdapterIterator<ValueType> aaIt;
  ~FixedSizeStorageIterator() {}
  FixedSizeStorageIterator(const VectorAdapterIterator<ValueType> &other)
      : vaIt(other) {}
  FixedSizeStorageIterator(
      const PersistentVectorAdapterIterator<ValueType> &other)
      : pvaIt(other) {}
  FixedSizeStorageIterator(const ArrayAdapterIterator<ValueType> &other)
      : aaIt(other) {}
};

template <typename ValueType> struct FixedSizeStorageAdapter {
  using value_ty = ValueType;
  class iterator {
    FixedSizeStorageIteratorKind kind;
    FixedSizeStorageIterator<ValueType> impl;

  public:
    iterator(const VectorAdapterIterator<ValueType> &impl)
        : kind(FixedSizeStorageIteratorKind::Vector), impl(impl) {}
    iterator(const PersistentVectorAdapterIterator<ValueType> &impl)
        : kind(FixedSizeStorageIteratorKind::PersistentVector), impl(impl) {}
    iterator(const ArrayAdapterIterator<ValueType> &impl)
        : kind(FixedSizeStorageIteratorKind::Array), impl(impl) {}
    iterator(iterator const &right) : kind(right.kind) {
      switch (kind) {
      case klee::FixedSizeStorageIteratorKind::Vector: {
        impl.vaIt = right.impl.vaIt;
        break;
      }
      case klee::FixedSizeStorageIteratorKind::PersistentVector: {
        impl.pvaIt = right.impl.pvaIt;
        break;
      }
      case klee::FixedSizeStorageIteratorKind::Array: {
        impl.aaIt = right.impl.aaIt;
        break;
      }
      default:
        assert(0 && "unhandled iterator kind");
        unreachable();
      }
    }
    ~iterator() {
      switch (kind) {
      case klee::FixedSizeStorageIteratorKind::Vector: {
        impl.vaIt.~VectorAdapterIterator();
        break;
      }
      case klee::FixedSizeStorageIteratorKind::PersistentVector: {
        impl.pvaIt.~PersistentVectorAdapterIterator();
        break;
      }
      case klee::FixedSizeStorageIteratorKind::Array: {
        impl.aaIt.~ArrayAdapterIterator();
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
      case klee::FixedSizeStorageIteratorKind::Vector: {
        ++impl.vaIt;
        break;
      }
      case klee::FixedSizeStorageIteratorKind::PersistentVector: {
        ++impl.pvaIt;
        break;
      }
      case klee::FixedSizeStorageIteratorKind::Array: {
        ++impl.aaIt;
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
      case klee::FixedSizeStorageIteratorKind::Vector: {
        return *impl.vaIt;
      }
      case klee::FixedSizeStorageIteratorKind::PersistentVector: {
        return *impl.pvaIt;
      }
      case klee::FixedSizeStorageIteratorKind::Array: {
        return *impl.aaIt;
      }
      default:
        assert(0 && "unhandled iterator kind");
        unreachable();
      }
    }
    bool operator!=(const iterator &other) const {
      switch (kind) {
      case klee::FixedSizeStorageIteratorKind::Vector: {
        return impl.vaIt != other.impl.vaIt;
      }
      case klee::FixedSizeStorageIteratorKind::PersistentVector: {
        return impl.pvaIt != other.impl.pvaIt;
      }
      case klee::FixedSizeStorageIteratorKind::Array: {
        return impl.aaIt != other.impl.aaIt;
      }
      default:
        assert(0 && "unhandled iterator kind");
        unreachable();
      }
    }
  };

  virtual ~FixedSizeStorageAdapter() = default;
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

  VectorAdapter(size_t storageSize) { storage = storage_ty(storageSize); }
  VectorAdapter(const VectorAdapter<ValueType> &va) : storage(va.storage) {}
  storage_ty storage;
  iterator begin() const override {
    return iterator(VectorAdapterIterator<ValueType>(storage.begin()));
  }
  iterator end() const override {
    return iterator(VectorAdapterIterator<ValueType>(storage.end()));
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
  using storage_ty = immer::vector_transient<ValueType>;
  using storage_persistent_ty = immer::vector<ValueType>;
  using base_ty = FixedSizeStorageAdapter<ValueType>;
  using iterator = typename base_ty::iterator;

  PersistentVectorAdapter(size_t storageSize) {
    storage = storage_persistent_ty(storageSize).transient();
  }
  PersistentVectorAdapter(const PersistentVectorAdapter<ValueType> &va)
      : storage(va.storage.persistent().transient()) {}
  PersistentVectorAdapter &operator=(const PersistentVectorAdapter &b) {
    storage = b.storage.persistent().transient();
    return *this;
  }
  mutable storage_ty storage;
  iterator begin() const override {
    return iterator(
        PersistentVectorAdapterIterator<ValueType>(storage.begin()));
  }
  iterator end() const override {
    return iterator(PersistentVectorAdapterIterator<ValueType>(storage.end()));
  }
  bool empty() const override { return storage.empty(); }
  void set(size_t key, const ValueType &value) override {
    storage.set(key, value);
  }
  const ValueType &at(size_t key) const override { return storage.at(key); }
  size_t size() const override { return storage.size(); }
  FixedSizeStorageAdapter<ValueType> *clone() const override {
    return new PersistentVectorAdapter<ValueType>(*this);
  }
};

template <typename ValueType>
struct ArrayAdapter : public FixedSizeStorageAdapter<ValueType> {
  using storage_ty = ValueType *;
  using base_ty = FixedSizeStorageAdapter<ValueType>;
  using iterator = typename base_ty::iterator;

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
  iterator begin() const override {
    return ArrayAdapterIterator<ValueType>(storage);
  }
  iterator end() const override {
    return ArrayAdapterIterator<ValueType>(storage + storageSize);
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

} // namespace klee

#endif
