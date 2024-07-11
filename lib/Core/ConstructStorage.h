//===-- ConstructStorage.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CONSTRUCT_STORAGE_H
#define KLEE_CONSTRUCT_STORAGE_H

#include "klee/ADT/FixedSizeStorageAdapter.h"
#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Expr.h"
#include "klee/Support/CompilerWarning.h"

#include <cassert>
#include <functional>

namespace klee {
enum class MemoryType { Fixed, Dynamic, Persistent, Mixed };

extern llvm::cl::opt<MemoryType> MemoryBackend;
extern llvm::cl::opt<unsigned long> MaxFixedSizeStructureSize;

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
SparseStorage<ValueType, Eq> *
constructStorage(ref<Expr> size, const ValueType &defaultValue,
                 size_t treshold = MaxFixedSizeStructureSize) {
  switch (MemoryBackend) {
  case klee::MemoryType::Mixed: {
    if (auto constSize = dyn_cast<ConstantExpr>(size);
        constSize && constSize->getZExtValue() <= treshold) {
      return new SparseStorageImpl<ValueType, Eq,
                                   SparseArrayAdapter<ValueType, Eq>>(
          defaultValue, typename SparseArrayAdapter<ValueType, Eq>::constructor(
                            constSize->getZExtValue()));
    } else {
      return new SparseStorageImpl<ValueType, Eq,
                                   PersistenUnorderedMapAdapder<ValueType, Eq>>(
          defaultValue);
    }
  }
  case klee::MemoryType::Fixed: {
    if (auto constSize = dyn_cast<ConstantExpr>(size); constSize) {
      return new SparseStorageImpl<ValueType, Eq,
                                   SparseArrayAdapter<ValueType, Eq>>(
          defaultValue, typename SparseArrayAdapter<ValueType, Eq>::constructor(
                            constSize->getZExtValue()));
    } else {
      return new SparseStorageImpl<ValueType, Eq,
                                   UnorderedMapAdapder<ValueType, Eq>>(
          defaultValue);
    }
  }
  case klee::MemoryType::Dynamic: {
    return new SparseStorageImpl<ValueType, Eq,
                                 UnorderedMapAdapder<ValueType, Eq>>(
        defaultValue);
  }
  case klee::MemoryType::Persistent: {
    return new SparseStorageImpl<ValueType, Eq,
                                 PersistenUnorderedMapAdapder<ValueType, Eq>>(
        defaultValue);
  }
  default:
    assert(0 && "unhandled memory type");
    unreachable();
  }
}

template <typename ValueType>
FixedSizeStorageAdapter<ValueType> *
constructStorage(size_t size, size_t treshold = MaxFixedSizeStructureSize) {
  switch (MemoryBackend) {
  case klee::MemoryType::Mixed: {
    if (size <= treshold) {
      return new ArrayAdapter<ValueType>(size);
    } else {
      return new PersistentVectorAdapter<ValueType>(size);
    }
  }
  case klee::MemoryType::Fixed: {
    return new ArrayAdapter<ValueType>(size);
  }
  case klee::MemoryType::Dynamic: {
    return new VectorAdapter<ValueType>(size);
  }
  case klee::MemoryType::Persistent: {
    return new PersistentVectorAdapter<ValueType>(size);
  }
  default:
    assert(0 && "unhandled memory type");
    unreachable();
  }
}
} // namespace klee

#endif /* KLEE_CONSTRUCT_STORAGE_H */
