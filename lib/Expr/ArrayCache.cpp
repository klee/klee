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
  for (ArrayPtrVec::iterator ai = concreteArrays.begin(),
                             e = concreteArrays.end();
       ai != e; ++ai) {
    delete *ai;
  }
}

const Array *ArrayCache::CreateArray(ref<Expr> _size,
                                     ref<SymbolicSource> _source,
                                     Expr::Width _domain, Expr::Width _range) {

  const Array *array = new Array(_size, _source, _domain, _range, getNextID());
  if (array->isSymbolicArray()) {
    std::pair<ArrayHashSet::const_iterator, bool> success =
        cachedSymbolicArrays.insert(array);
    if (success.second) {
      // Cache miss
      return array;
    }
    // Cache hit
    delete array;
    array = *(success.first);
    assert(array->isSymbolicArray() &&
           "Cached symbolic array is no longer symbolic");
    return array;
  } else {
    // Treat every constant array as distinct so we never cache them
    assert(array->isConstantArray());
    concreteArrays.push_back(array); // For deletion later
    return array;
  }
}

unsigned ArrayCache::getNextID() const {
  return cachedSymbolicArrays.size() + concreteArrays.size();
}

} // namespace klee
