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

#include <deque>

#include <sys/resource.h>

// This test is disabled for asan and msan because they create additional page
// faults
#if !defined(__has_feature) ||                                                 \
    (!__has_feature(memory_sanitizer) && !__has_feature(address_sanitizer))

std::size_t write_to_allocations(std::deque<void *> &allocations) {
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
  // initialize a factory and an associated allocator (using the location "0"
  // gives an OS-assigned location)
  klee::kdalloc::AllocatorFactory factory(static_cast<std::size_t>(1) << 30,
                                          0); // 1 GB
  klee::kdalloc::Allocator allocator = factory.makeAllocator();

  std::deque<void *> allocations;
  for (std::size_t i = 0; i < 1000; ++i) {
    auto p = allocator.allocate(16);
    allocations.emplace_back(p);
  }

  auto pageFaults = write_to_allocations(allocations);

  ASSERT_GT(pageFaults, static_cast<std::size_t>(0))
      << "No page faults happened";
  ASSERT_EQ(pageFaults, allocations.size())
      << "Number of (minor) page faults and objects differs";

  factory.getMapping().clear();

  // try again: this should (again) trigger a minor page fault for every object
  auto pageFaultsSecondTry = write_to_allocations(allocations);

  ASSERT_EQ(pageFaults, pageFaultsSecondTry)
      << "Number of minor page faults in second try differs";
}

#endif
