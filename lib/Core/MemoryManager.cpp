//===-- MemoryManager.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MemoryManager.h"
#include "CoreStats.h"
#include "Memory.h"

#include "klee/Expr.h"
#include "klee/Internal/Support/ErrorHandling.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MathExtras.h"

#include <cstdint>
#include <limits>
#include <sys/mman.h>

using namespace klee;

namespace {
llvm::cl::opt<bool> DeterministicAllocation(
    "allocate-determ",
    llvm::cl::desc("Allocate memory deterministically(default=off)"),
    llvm::cl::init(false));

llvm::cl::opt<std::size_t> DeterministicAllocationSize(
    "allocate-determ-size",
    llvm::cl::desc(
        "Preallocated memory for deterministic allocation in MB (default=100)"),
    llvm::cl::init(100));

llvm::cl::opt<bool>
    NullOnZeroMalloc("return-null-on-zero-malloc",
                     llvm::cl::desc("Returns NULL in case malloc(size) was "
                                    "called with size 0 (default=off)."),
                     llvm::cl::init(false));

llvm::cl::opt<std::size_t> RedZoneSpace(
    "red-zone-space",
    llvm::cl::desc("Set the amount of free space between allocations. This is "
                   "important to detect out-of-bound accesses (default=10)."),
    llvm::cl::init(10));

llvm::cl::opt<std::uintptr_t> DeterministicStartAddress(
    "allocate-determ-start-address",
    llvm::cl::desc("Start address for deterministic allocation. Has to be page "
                   "aligned (default=0x7ff30000000)."),
    llvm::cl::init(0x7ff30000000));
} // namespace

MemoryManager::MemoryManager(ArrayCache *_arrayCache)
    : arrayCache(_arrayCache), deterministicSpace(0), nextFreeSlot(0),
      spaceSize(DeterministicAllocationSize.getValue() * 1024 * 1024) {
  if (DeterministicAllocation) {
    // Page boundary
    void *expectedAddress =
        reinterpret_cast<void *>(DeterministicStartAddress.getValue());

    void *newSpace = mmap(expectedAddress, spaceSize, PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (newSpace == MAP_FAILED) {
      klee_error("Couldn't mmap() memory for deterministic allocations");
    }
    if (expectedAddress != newSpace && expectedAddress != nullptr) {
      klee_error("Could not allocate memory deterministically");
    }

    klee_message("Deterministic memory allocation starting from %p", newSpace);
    deterministicSpace = reinterpret_cast<std::uintptr_t>(newSpace);
    nextFreeSlot = reinterpret_cast<std::uintptr_t>(newSpace);
  }
}

MemoryManager::~MemoryManager() {
  while (!objects.empty()) {
    MemoryObject *mo = *objects.begin();
    if (!mo->isFixed && !DeterministicAllocation) {
      free(reinterpret_cast<void *>(mo->address));
    }
    objects.erase(mo);
    delete mo;
  }

  if (DeterministicAllocation) {
    munmap(reinterpret_cast<void *>(deterministicSpace), spaceSize);
  }
}

MemoryObject *MemoryManager::allocate(std::size_t size, bool isLocal,
                                      bool isGlobal,
                                      const llvm::Value *allocSite,
                                      std::size_t alignment) {
  static_assert(std::numeric_limits<std::size_t>::digits <=
                    std::numeric_limits<std::uint64_t>::digits,
                "Several LLVM functions used by the memory allocator assume "
                "that the address space has at most 64 bits");

  if (size > 10 * 1024 * 1024)
    klee_warning_once(0, "Large alloc: %zu bytes.  KLEE may run out of memory.",
                      size);

  // Return NULL if size is zero
  if (NullOnZeroMalloc && size == 0)
    return 0;

  if (!llvm::isPowerOf2_64(alignment)) {
    klee_warning("Only alignment of power of two is supported");
    return 0;
  }

  std::uintptr_t address = 0;
  if (DeterministicAllocation) {
    address = llvm::RoundUpToAlignment(nextFreeSlot + alignment - 1, alignment);

    // Handle the case of 0-sized allocations as 1-byte allocations.
    // This way, we make sure we have this allocation between its own red zones
    std::size_t alloc_size = std::max(size, static_cast<std::size_t>(1));
    if (address + alloc_size < deterministicSpace + spaceSize) {
      nextFreeSlot = address + alloc_size + RedZoneSpace;
    } else {
      klee_warning_once(
          0,
          "Couldn't allocate %zu bytes. Not enough deterministic space left.",
          size);
      address = 0;
    }
  } else {
    // Use malloc for the standard case
    if (alignment <= 8)
      address = reinterpret_cast<std::uintptr_t>(malloc(size));
    else {
      void *addressPtr = nullptr;
      if (posix_memalign(&addressPtr, alignment, size)) {
        klee_warning("Allocating aligned memory failed.");
        address = 0;
      } else {
        address = reinterpret_cast<std::uintptr_t>(addressPtr);
      }
    }
  }

  if (!address)
    return 0;

  ++stats::allocations;
  MemoryObject *res = new MemoryObject(address, size, isLocal, isGlobal, false,
                                       allocSite, this);
  objects.insert(res);
  return res;
}

MemoryObject *MemoryManager::allocateFixed(std::uintptr_t address,
                                           std::size_t size,
                                           const llvm::Value *allocSite) {
#ifndef NDEBUG
  for (objects_ty::iterator it = objects.begin(), ie = objects.end(); it != ie;
       ++it) {
    MemoryObject *mo = *it;
    if (address + size > mo->address && address < mo->address + mo->size)
      klee_error("Trying to allocate an overlapping object");
  }
#endif

  ++stats::allocations;
  MemoryObject *res =
      new MemoryObject(address, size, false, true, true, allocSite, this);
  objects.insert(res);
  return res;
}

void MemoryManager::deallocate(const MemoryObject *mo) { assert(0); }

void MemoryManager::markFreed(MemoryObject *mo) {
  if (objects.find(mo) != objects.end()) {
    if (!mo->isFixed && !DeterministicAllocation) {
      free(reinterpret_cast<void *>(mo->address));
    }
    objects.erase(mo);
  }
}

std::size_t MemoryManager::getUsedDeterministicSize() {
  return nextFreeSlot - deterministicSpace;
}
