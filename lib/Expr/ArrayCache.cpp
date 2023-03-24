#include "klee/Expr/ArrayCache.h"

namespace klee {
const Array *
ArrayCache::CreateArray(const std::string &_name, std::uint64_t _size,
                        const ref<ConstantExpr> *constantValuesBegin,
                        const ref<ConstantExpr> *constantValuesEnd,
                        Expr::Width _domain, Expr::Width _range) {
  if (constantValuesBegin == constantValuesEnd) {
    auto it = symbolicArrays
                  .emplace(Array::Private{}, _name, _size, nullptr, nullptr,
                           _domain, _range)
                  .first;
    auto array = &*it;
    assert(array->isSymbolicArray() && "Array should be symbolic");
    return array;
  } else {
    // Treat every constant array as distinct so we never cache them
    concreteArrays.emplace_front(Array::Private{}, _name, _size,
                                 constantValuesBegin, constantValuesEnd,
                                 _domain, _range);
    auto array = &concreteArrays.front();
    assert(array->isConstantArray() && "Array should be constant");
    return array;
  }
}
} // namespace klee
