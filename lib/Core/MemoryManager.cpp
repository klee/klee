//===-- MemoryManager.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MemoryManager.h"
#include "AddressManager.h"

#include "CoreStats.h"
#include "ExecutionState.h"
#include "Memory.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/SourceBuilder.h"
#include "klee/Support/ErrorHandling.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MathExtras.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(10, 0)
#include "llvm/Support/Alignment.h"
#else
#include "llvm/Support/MathExtras.h"
#endif
DISABLE_WARNING_POP

#include <algorithm>
#include <cinttypes>
#include <string>
#include <sys/mman.h>
#include <tuple>

using namespace klee;

namespace {
llvm::cl::OptionCategory MemoryCat("Memory management options",
                                   "These options control memory management.");

llvm::cl::opt<bool> DeterministicAllocation(
    "allocate-determ",
    llvm::cl::desc("Allocate memory deterministically (default=false)"),
    llvm::cl::init(false), llvm::cl::cat(MemoryCat));

llvm::cl::opt<unsigned> DeterministicAllocationSize(
    "allocate-determ-size",
    llvm::cl::desc(
        "Preallocated memory for deterministic allocation in MB (default=100)"),
    llvm::cl::init(100), llvm::cl::cat(MemoryCat));

llvm::cl::opt<bool> NullOnZeroMalloc(
    "return-null-on-zero-malloc",
    llvm::cl::desc("Returns NULL if malloc(0) is called (default=false)"),
    llvm::cl::init(false), llvm::cl::cat(MemoryCat));

llvm::cl::opt<unsigned> RedzoneSize(
    "redzone-size",
    llvm::cl::desc("Set the size of the redzones to be added after each "
                   "allocation (in bytes). This is important to detect "
                   "out-of-bounds accesses (default=10)"),
    llvm::cl::init(10), llvm::cl::cat(MemoryCat));

llvm::cl::opt<unsigned long long> DeterministicStartAddress(
    "allocate-determ-start-address",
    llvm::cl::desc("Start address for deterministic allocation. Has to be page "
                   "aligned (default=0x7ff30000000)"),
    llvm::cl::init(0x7ff30000000), llvm::cl::cat(MemoryCat));
} // namespace

llvm::cl::opt<uint64_t> MaxConstantAllocationSize(
    "max-constant-alloc",
    llvm::cl::desc(
        "Maximum available size for single allocation (default 10Mb)"),
    llvm::cl::init(10ll << 20), llvm::cl::cat(MemoryCat));

llvm::cl::opt<uint64_t> MaxSymbolicAllocationSize(
    "max-sym-alloc",
    llvm::cl::desc(
        "Maximum available size for single allocation (default 10Mb)"),
    llvm::cl::init(10ll << 20), llvm::cl::cat(MemoryCat));

llvm::cl::opt<bool> UseSymbolicSizeAllocation(
    "use-sym-size-alloc",
    llvm::cl::desc("Allows symbolic size allocation (default false)"),
    llvm::cl::init(false), llvm::cl::cat(MemoryCat));

/***/
MemoryManager::MemoryManager(ArrayCache *_arrayCache)
    : arrayCache(_arrayCache), deterministicSpace(0), nextFreeSlot(0),
      spaceSize(DeterministicAllocationSize.getValue() * 1024 * 1024) {
  if (DeterministicAllocation) {
    // Page boundary
    void *expectedAddress = (void *)DeterministicStartAddress.getValue();

    char *newSpace =
        (char *)mmap(expectedAddress, spaceSize, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (newSpace == MAP_FAILED) {
      klee_error("Couldn't mmap() memory for deterministic allocations");
    }
    if (expectedAddress != newSpace && expectedAddress != 0) {
      klee_error("Could not allocate memory deterministically");
    }

    klee_message("Deterministic memory allocation starting from %p", newSpace);
    deterministicSpace = newSpace;
    nextFreeSlot = newSpace;
  }
}

MemoryManager::~MemoryManager() {
  while (!objects.empty()) {
    MemoryObject *mo = *objects.begin();
    if (!mo->isFixed && !DeterministicAllocation)
      free((void *)mo->address);
    objects.erase(mo);
    delete mo;
  }

  if (DeterministicAllocation)
    munmap(deterministicSpace, spaceSize);
}

MemoryObject *MemoryManager::allocate(uint64_t size, bool isLocal,
                                      bool isGlobal, bool isLazyInitialiazed,
                                      ref<CodeLocation> allocSite,
                                      size_t alignment, ref<Expr> addressExpr,
                                      ref<Expr> sizeExpr, unsigned timestamp,
                                      IDType id) {
  if (size > MaxConstantAllocationSize) {
    klee_warning_once(
        0, "Large alloc: %" PRIu64 " bytes.  KLEE models out of memory.", size);
    return 0;
  }

  // Return NULL if size is zero, this is equal to error during allocation
  if (NullOnZeroMalloc && size == 0)
    return 0;

  if (!llvm::isPowerOf2_64(alignment)) {
    klee_warning("Only alignment of power of two is supported");
    return 0;
  }

  uint64_t address = 0;
  if (DeterministicAllocation) {
    address = llvm::alignTo((uint64_t)nextFreeSlot + alignment - 1, alignment);

    // Handle the case of 0-sized allocations as 1-byte allocations.
    // This way, we make sure we have this allocation between its own red zones
    size_t alloc_size = std::max(size, (uint64_t)1);
    if ((char *)address + alloc_size < deterministicSpace + spaceSize) {
      nextFreeSlot = (char *)address + alloc_size + RedzoneSize;
    } else {
      klee_warning_once(0,
                        "Couldn't allocate %" PRIu64
                        " bytes. Not enough deterministic space left.",
                        size);
      address = 0;
    }
  } else {
    // Use malloc for the standard case
    if (alignment <= 8)
      address = (uint64_t)malloc(size);
    else {
      int res = posix_memalign((void **)&address, alignment, size);
      if (res < 0) {
        klee_warning("Allocating aligned memory failed.");
        address = 0;
      }
    }
  }

  if (!address)
    return 0;

  ++stats::allocations;
  MemoryObject *res = new MemoryObject(
      address, size, alignment, isLocal, isGlobal, false, isLazyInitialiazed,
      allocSite, this, addressExpr, sizeExpr, timestamp);
  if (id) {
    res->id = id;
  }
  allocatedSizes[res->id][size] = res;

  objects.insert(res);
  return res;
}

MemoryObject *MemoryManager::allocateFixed(uint64_t address, uint64_t size,
                                           ref<CodeLocation> allocSite) {
#ifndef NDEBUG
  for (objects_ty::iterator it = objects.begin(), ie = objects.end(); it != ie;
       ++it) {
    MemoryObject *mo = *it;
    if (address + size > mo->address && address < mo->address + mo->size)
      klee_error("Trying to allocate an overlapping object");
  }
#endif

  ++stats::allocations;
  MemoryObject *res = new MemoryObject(address, size, 8, false, true, true,
                                       false, allocSite, this);
  objects.insert(res);
  return res;
}

void MemoryManager::deallocate(const MemoryObject *mo) { assert(0); }

void MemoryManager::markFreed(MemoryObject *mo) {
  if (objects.find(mo) != objects.end()) {
    allocatedSizes[mo->id].erase(mo->size);
    if (allocatedSizes[mo->id].empty()) {
      am->bindingsAdressesToObjects.erase(mo->getBaseExpr());
    }
    if (!mo->isFixed && !DeterministicAllocation)
      free((void *)mo->address);
    objects.erase(mo);
  }
}

const std::map<uint64_t, MemoryObject *> &
MemoryManager::getAllocatedObjects(IDType idObject) {
  return allocatedSizes[idObject];
}

size_t MemoryManager::getUsedDeterministicSize() {
  return nextFreeSlot - deterministicSpace;
}
