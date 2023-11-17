#include "AddressManager.h"

#include "MemoryManager.h"
#include "klee/Expr/Expr.h"

namespace klee {

void AddressManager::addAllocation(ref<Expr> address, IDType id) {
  assert(!bindingsAdressesToObjects.count(address));
  bindingsAdressesToObjects[address] = id;
}

void *AddressManager::allocate(ref<Expr> address, uint64_t size) {
  IDType id = bindingsAdressesToObjects.at(address);

  auto &objects = memory->allocatedSizes.at(id);
  auto sizeLocation = objects.lower_bound(size);
  MemoryObject *newMO;
  if (size > maxSize) {
    if (sizeLocation != objects.end()) {
      newMO = sizeLocation->second;
    } else {
      newMO = nullptr;
      objects[size] = newMO;
    }
  } else {
    if (sizeLocation == objects.end() || !sizeLocation->second) {
      uint64_t newSize = std::max((uint64_t)1, size);
      int clz = __builtin_clzll(std::max((uint64_t)1, newSize));
      int popcount = __builtin_popcountll(std::max((uint64_t)1, newSize));
      if (popcount > 1) {
        newSize = (uint64_t)1 << (sizeof(newSize) * CHAR_BIT - clz);
      }
      if (newSize > maxSize) {
        newSize = std::max((uint64_t)1, size);
      }
      assert(!objects.empty());
      MemoryObject *mo = objects.begin()->second;
      newMO = memory->allocate(newSize, mo->isLocal, mo->isGlobal,
                               mo->isLazyInitialized, mo->allocSite,
                               mo->alignment, mo->addressExpr, mo->sizeExpr,
                               mo->timestamp, mo->id);
    } else {
      newMO = sizeLocation->second;
    }
  }
  if (newMO) {
    assert(size <= newMO->size);
    return reinterpret_cast<void *>(newMO->address);
  } else {
    return nullptr;
  }
}

MemoryObject *AddressManager::allocateMemoryObject(ref<Expr> address,
                                                   uint64_t size) {
  IDType id = bindingsAdressesToObjects.at(address);
  const auto &objects = memory->getAllocatedObjects(id);
  auto resultIterator = objects.lower_bound(size);
  if (resultIterator == objects.end()) {
    allocate(address, size);
    resultIterator = objects.lower_bound(size);
  }
  assert(resultIterator != objects.end());
  return resultIterator->second;
}

bool AddressManager::isAllocated(ref<Expr> address) {
  return bindingsAdressesToObjects.count(address);
}

} // namespace klee
