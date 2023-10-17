//===-- ArrayCache.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_ARRAYCACHE_H
#define KLEE_ARRAYCACHE_H

#include "klee/Expr/ArrayExprHash.h" // For klee::ArrayHashFn
#include "klee/Expr/Expr.h"
#include "klee/Expr/SymbolicSource.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace klee {

struct EquivArrayCmpFn {
  bool operator()(const Array *array1, const Array *array2) const {
    if (array1 == NULL || array2 == NULL)
      return false;
    return (array1->size == array2->size) && (array1->source == array2->source);
  }
};

/// Provides an interface for creating and destroying Array objects.
class ArrayCache {
public:
  template <class T>
  class ArrayHashMap
      : public std::unordered_map<const Array *, T, klee::ArrayHashFn,
                                  klee::EquivArrayCmpFn> {};

  ArrayCache() {}
  ~ArrayCache();
  /// Create an Array object.
  //
  /// Symbolic Arrays are cached so that only one instance exists. This
  /// provides a limited form of "alpha-renaming". Constant arrays are not
  /// cached.
  ///
  /// This class retains ownership of Array object so that upon destruction
  /// of this object all allocated Array objects are deleted.
  ///
  /// \param _name The name of the array
  /// \param _size The size of the array in bytes
  /// \param _domain The size of the domain (i.e. the bitvector used to index
  /// the array)
  /// \param _range The size of range (i.e. the bitvector that is indexed to)
  const Array *CreateArray(ref<Expr> _size, const ref<SymbolicSource> source,
                           Expr::Width _domain = Expr::Int32,
                           Expr::Width _range = Expr::Int8);

private:
  typedef std::unordered_set<const Array *, klee::ArrayHashFn,
                             klee::EquivArrayCmpFn>
      ArrayHashSet;
  ArrayHashSet cachedSymbolicArrays;

  // Number of arrays of each source allocated
  std::unordered_map<SymbolicSource::Kind, unsigned> allocatedCount;
};
} // namespace klee

#endif /* KLEE_ARRAYCACHE_H */
