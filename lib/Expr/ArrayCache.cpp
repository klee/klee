#include "klee/Expr/ArrayCache.h"

#include "klee/Expr/SymbolicSource.h"

namespace klee {

ArrayCache::~ArrayCache() {
  // Free Allocated Array objects
  for (ArrayHashSet::iterator ai = cachedSymbolicArrays.begin(),
                              e = cachedSymbolicArrays.end();
       ai != e; ++ai) {
    delete *ai;
  }
}

const Array *ArrayCache::CreateArray(ref<Expr> _size,
                                     ref<SymbolicSource> _source,
                                     Expr::Width _domain, Expr::Width _range) {

  auto id = allocatedCount[_source->getKind()];
  const Array *array = new Array(_size, _source, _domain, _range, id);
  std::pair<ArrayHashSet::const_iterator, bool> success =
      cachedSymbolicArrays.insert(array);
  if (success.second) {
    // Cache miss
    allocatedCount[_source->getKind()]++;
    return array;
  }
  // Cache hit
  delete array;
  array = *(success.first);
  return array;
}

} // namespace klee
