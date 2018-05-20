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

#include <cstdint>
#include <set>

namespace llvm {
class Value;
}

namespace klee {
class MemoryObject;
class ArrayCache;

class MemoryManager {
private:
  typedef std::set<MemoryObject *> objects_ty;
  objects_ty objects;
  ArrayCache *const arrayCache;

  std::uintptr_t deterministicSpace;
  std::uintptr_t nextFreeSlot;
  std::size_t spaceSize;

public:
  MemoryManager(ArrayCache *arrayCache);
  ~MemoryManager();

  /**
   * Returns memory object which contains a handle to real virtual process
   * memory.
   */
  MemoryObject *allocate(std::size_t size, bool isLocal, bool isGlobal,
                         const llvm::Value *allocSite, std::size_t alignment);
  MemoryObject *allocateFixed(std::uintptr_t address, std::size_t size,
                              const llvm::Value *allocSite);
  void deallocate(const MemoryObject *mo);
  void markFreed(MemoryObject *mo);
  ArrayCache *getArrayCache() const { return arrayCache; }

  /*
   * Returns the size used by deterministic allocation in bytes
   */
  std::size_t getUsedDeterministicSize();
};

} // namespace klee

#endif
