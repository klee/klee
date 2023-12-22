//===-- MemoryManager.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_MEMORYMANAGER_H
#define KLEE_MEMORYMANAGER_H

#include "klee/Expr/Expr.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_map>

namespace llvm {
class Value;
}

namespace klee {
class MemoryObject;
class ArrayCache;
class AddressManager;
struct CodeLocation;

typedef uint64_t IDType;

class MemoryManager {
  friend class AddressManager;

private:
  typedef std::set<MemoryObject *> objects_ty;
  objects_ty objects;
  std::unordered_map<IDType, std::map<uint64_t, MemoryObject *>> allocatedSizes;

  ArrayCache *const arrayCache;

  char *deterministicSpace;
  char *nextFreeSlot;
  size_t spaceSize;

public:
  AddressManager *am;
  MemoryManager(ArrayCache *arrayCache);
  ~MemoryManager();

  /**
   * Returns memory object which contains a handle to real virtual process
   * memory.
   */
  MemoryObject *allocate(uint64_t size, bool isLocal, bool isGlobal,
                         bool isLazyInitialiazed, ref<CodeLocation> allocSite,
                         size_t alignment, ref<Expr> addressExpr = ref<Expr>(),
                         ref<Expr> sizeExpr = ref<Expr>(),
                         unsigned timestamp = 0, IDType id = 0);
  MemoryObject *allocateFixed(uint64_t address, uint64_t size,
                              ref<CodeLocation> allocSite);
  void deallocate(const MemoryObject *mo);
  void markFreed(MemoryObject *mo);
  ArrayCache *getArrayCache() const { return arrayCache; }
  const std::map<uint64_t, MemoryObject *> &
  getAllocatedObjects(IDType idObject);
  /*
   * Returns the size used by deterministic allocation in bytes
   */
  size_t getUsedDeterministicSize();
};

} // namespace klee

#endif /* KLEE_MEMORYMANAGER_H */
