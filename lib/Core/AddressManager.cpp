#include "AddressManager.h"

#include "MemoryManager.h"
#include "klee/Expr/Expr.h"

namespace klee {

void AddressManager::addAllocation(const Array *array, IDType id) {
  bindingsArraysToObjects[array] = id;
}

void *AddressManager::allocate(const Array *array, uint64_t size) {
  IDType id = bindingsArraysToObjects.at(array);

  const auto &objects = memory->getAllocatedObjects(id);
  auto sizeLocation = objects.lower_bound(size);
  MemoryObject *newMO;
  if (sizeLocation == objects.end()) {
    uint64_t newSize = (uint64_t)1
                       << (sizeof(size) * CHAR_BIT -
                           __builtin_clzll(std::max((uint64_t)1, size)));
    assert(!objects.empty());
    MemoryObject *mo = std::prev(objects.end())->second;
    newMO =
        memory->allocate(newSize, mo->isLocal, mo->isGlobal, mo->allocSite,
                         mo->alignment, mo->addressExpr, mo->sizeExpr,
                         mo->lazyInitializationSource, mo->timestamp, mo->id);
  } else {
    newMO = sizeLocation->second;
  }
  assert(size <= newMO->size);
  return reinterpret_cast<void *>(newMO->address);
}

MemoryObject *AddressManager::allocateMemoryObject(const Array *array,
                                                   uint64_t size) {
  IDType id = bindingsArraysToObjects.at(array);
  const auto &objects = memory->getAllocatedObjects(id);
  auto resultIterator = objects.lower_bound(size);
  assert(resultIterator != objects.end());
  return resultIterator->second;
}

} // namespace klee
