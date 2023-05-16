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

#include "klee/KDAlloc/kdalloc.h"

#include <cstddef>
#include <set>
#include <cstdint>

namespace llvm {
class Value;
}

namespace klee {
class ArrayCache;
class ExecutionState;
class MemoryObject;

class MemoryManager {
private:
  typedef std::set<MemoryObject *> objects_ty;
  objects_ty objects;
  ArrayCache *const arrayCache;

  kdalloc::AllocatorFactory globalsFactory;
  kdalloc::Allocator globalsAllocator;

  kdalloc::AllocatorFactory constantsFactory;
  kdalloc::Allocator constantsAllocator;

public:
  explicit MemoryManager(ArrayCache *arrayCache);
  ~MemoryManager();

  kdalloc::AllocatorFactory heapFactory;
  kdalloc::StackAllocatorFactory stackFactory;

  static std::uint32_t quarantine;

  static std::size_t pageSize;

  static bool isDeterministic;

  /**
   * Returns memory object which contains a handle to real virtual process
   * memory.
   */
  MemoryObject *allocate(uint64_t size, bool isLocal, bool isGlobal,
                         ExecutionState *state, const llvm::Value *allocSite,
                         size_t alignment);
  MemoryObject *allocateFixed(uint64_t address, uint64_t size,
                              const llvm::Value *allocSite);
  void markFreed(MemoryObject *mo);
  bool markMappingsAsUnneeded();
  ArrayCache *getArrayCache() const { return arrayCache; }

  /*
   * Returns the size used by deterministic allocation in bytes
   */
  size_t getUsedDeterministicSize();
};

} // End klee namespace

#endif /* KLEE_MEMORYMANAGER_H */
