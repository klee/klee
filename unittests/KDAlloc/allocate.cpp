//===-- allocate.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/KDAlloc/kdalloc.h"

#if defined(USE_GTEST_INSTEAD_OF_MAIN)
#include "gtest/gtest.h"
#endif

#include <cassert>
#include <deque>
#include <iomanip>
#include <iostream>

void allocate_sample_test() {
  // initialize a factory and an associated allocator (1 MiB and no quarantine)
  klee::kdalloc::AllocatorFactory factory(static_cast<std::size_t>(1) << 20, 0);
  klee::kdalloc::Allocator allocator = factory.makeAllocator();

  std::deque<void *> allocations;
  for (std::size_t i = 0; i < 10; ++i) {
    auto p = allocator.allocate(4);
    allocations.emplace_back(p);
    std::cout << "Allocated     " << std::right << std::setw(64 / 4) << std::hex
              << (static_cast<char *>(p) -
                  static_cast<char *>(factory.getMapping().getBaseAddress()))
              << "\n";
  }

  {
    auto p = allocations[2];
    allocations.erase(allocations.begin() + 2);
    std::cout << "Freeing       " << std::right << std::setw(64 / 4) << std::hex
              << (static_cast<char *>(p) -
                  static_cast<char *>(factory.getMapping().getBaseAddress()))
              << "\n";
    allocator.free(p, 4);
  }

  for (auto p : allocations) {
    std::cout << "Freeing       " << std::right << std::setw(64 / 4) << std::hex
              << (static_cast<char *>(p) -
                  static_cast<char *>(factory.getMapping().getBaseAddress()))
              << "\n";
    allocator.free(p, 4);
  }

  exit(0);
}

#if defined(USE_GTEST_INSTEAD_OF_MAIN)
TEST(KDAllocDeathTest, AllocateSample) {
  ASSERT_EXIT(allocate_sample_test(), ::testing::ExitedWithCode(0), "");
}
#else
int main() { allocate_sample_test(); }
#endif
