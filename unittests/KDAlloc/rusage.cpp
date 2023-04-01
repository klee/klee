//===-- rusage.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/KDAlloc/kdalloc.h"

#include "gtest/gtest.h"

#include <vector>

#include <sys/resource.h>

// This test is disabled for asan and msan because they create additional page
// faults
#if !defined(__has_feature) ||                                                 \
    (!__has_feature(memory_sanitizer) && !__has_feature(address_sanitizer))

std::size_t write_to_allocations(std::vector<void *> &allocations) {
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  auto initial = ru.ru_minflt;

  for (auto p : allocations) {
    auto pp = static_cast<char *>(p);
    *pp = 1;
  }

  getrusage(RUSAGE_SELF, &ru);
  return ru.ru_minflt - initial;
}

TEST(KDAllocTest, Rusage) {
  // initialize a factory and an associated allocator (1 GiB and no quarantine)
  klee::kdalloc::AllocatorFactory factory(static_cast<std::size_t>(1) << 30, 0);
  klee::kdalloc::Allocator allocator = factory.makeAllocator();

  std::vector<void *> allocations;
  for (std::size_t i = 0; i < 1000; ++i) {
    allocations.emplace_back(allocator.allocate(16));
  }

  // When writing to allocations we should have at least one page fault per
  // object, but may have more due to unrelated behavior (e.g., swapping). In
  // the hopes that such interference is relatively rare, we try a few times to
  // encounter a perfect run, where the number of page faults matches the number
  // of allocated objects exactly.
  constexpr std::size_t tries = 100;
  for (std::size_t i = 0; i < tries; ++i) {
    auto pageFaults = write_to_allocations(allocations);
    if (pageFaults == allocations.size()) {
      // we have a perfect match
      return;
    }

    ASSERT_GE(pageFaults, allocations.size())
        << "There should be at least as many page faults as allocated objects";

    factory.getMapping().clear();
  }

  FAIL()
      << "No single try out of " << tries
      << " yielded a perfect match between page faults and allocated objects";
}

#endif
