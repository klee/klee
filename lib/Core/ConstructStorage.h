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

#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Expr.h"

#include <functional>

namespace klee {
extern llvm::cl::opt<bool> UseImmerStructures;

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
Storage<ValueType, Eq> *constructStorage(ref<Expr> size,
                                         const ValueType &defaultValue) {
  if (auto constSize = dyn_cast<ConstantExpr>(size);
      constSize && constSize->getZExtValue() <= 1024) {
    return new SparseStorage<ValueType, Eq, SparseArrayAdapter<ValueType, Eq>>(
        defaultValue, typename SparseArrayAdapter<ValueType, Eq>::allocator(
                          constSize->getZExtValue()));
  } else {
    if (UseImmerStructures) {
      return new SparseStorage<ValueType, Eq,
                               PersistenUnorderedMapAdapder<ValueType, Eq>>(
          defaultValue);
    } else {
      return new SparseStorage<ValueType, Eq,
                               UnorderedMapAdapder<ValueType, Eq>>(
          defaultValue);
    }
  }
}

template <typename ValueType>
FixedSizeStorageAdapter<ValueType> *constructStorage(size_t size) {
  if (UseImmerStructures) {
    return new PersistentVectorAdapter<ValueType>(size);
  } else {
    return new ArrayAdapter<ValueType>(size);
    // return new VectorAdapter<ValueType>(size);
  }
}
} // namespace klee

#endif /* KLEE_CONSTRUCT_STORAGE_H */
