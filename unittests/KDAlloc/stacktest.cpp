//===-- stacktest.cpp -----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/KDAlloc/kdalloc.h"
#include "xoshiro.h"

#if defined(USE_GTEST_INSTEAD_OF_MAIN)
#include "gtest/gtest.h"
#endif

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

namespace {
class RandomTest {
  xoshiro512 rng;

  klee::kdalloc::StackAllocator allocator;

  std::vector<std::pair<void *, std::size_t>> allocations;

  std::geometric_distribution<std::size_t> allocation_bin_distribution;
  std::geometric_distribution<std::size_t> large_allocation_distribution;

public:
  std::size_t maximum_concurrent_allocations = 0;
  std::uint64_t allocation_count = 0;
  std::uint64_t deallocation_count = 0;

  RandomTest(std::mt19937_64::result_type seed = 0x31337)
      : rng(seed), allocator(klee::kdalloc::StackAllocatorFactory(
                       static_cast<std::size_t>(1) << 42, 0)),
        allocation_bin_distribution(0.3),
        large_allocation_distribution(0.00003) {}

  void run(std::uint64_t const iterations) {
    allocations.reserve((iterations * 7) / 10);
    std::uniform_int_distribution<std::uint32_t> choice(0, 999);
    for (std::uint64_t i = 0; i < iterations; ++i) {
      auto chosen = choice(rng);
      if (chosen < 650) {
        ++allocation_count;
        allocate_sized();
      } else if (chosen < 700) {
        ++allocation_count;
        allocate_large();
      } else if (chosen < 1000) {
        ++deallocation_count;
        deallocate();
      }
    }
    cleanup();
  }

  void cleanup() {
    while (!allocations.empty()) {
      allocator.free(allocations.back().first, allocations.back().second);
      allocations.pop_back();
    }
  }

  void allocate_sized() {
    auto bin = allocation_bin_distribution(rng);
    while (bin >= 11) {
      bin = allocation_bin_distribution(rng);
    }
    auto min = (bin == 0 ? 1 : (static_cast<std::size_t>(1) << (bin + 1)) + 1);
    auto max = static_cast<std::size_t>(1) << (bin + 2);
    auto size = std::uniform_int_distribution<std::size_t>(min, max)(rng);

    allocations.emplace_back(allocator.allocate(size), size);
    if (allocations.size() > maximum_concurrent_allocations) {
      maximum_concurrent_allocations = allocations.size();
    }
  }

  void allocate_large() {
    auto size = 0;
    while (size <= 4096 || size > 1073741825) {
      size = large_allocation_distribution(rng) + 4097;
    }

    allocations.emplace_back(allocator.allocate(size), size);
    if (allocations.size() > maximum_concurrent_allocations) {
      maximum_concurrent_allocations = allocations.size();
    }
  }

  void deallocate() {
    if (allocations.empty()) {
      return;
    }
    allocator.free(allocations.back().first, allocations.back().second);
    allocations.pop_back();
  }
};
} // namespace

void stack_test() {
  RandomTest tester;
  tester.run(10'000'000);

  std::cout << "Allocations: " << tester.allocation_count << "\n";
  std::cout << "Deallocations: " << tester.deallocation_count << "\n";
  std::cout << "Maximum concurrent allocations: "
            << tester.maximum_concurrent_allocations << "\n";

  exit(0);
}

#if defined(USE_GTEST_INSTEAD_OF_MAIN)
TEST(KDAllocDeathTest, Stack) {
  ASSERT_EXIT(stack_test(), ::testing::ExitedWithCode(0), "");
}
#else
int main() { stack_test(); }
#endif
