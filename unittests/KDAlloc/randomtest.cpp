//===-- randomtest.cpp ----------------------------------------------------===//
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
#include <chrono>
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

#if defined(USE_KDALLOC)
  klee::kdalloc::Allocator allocator;
#endif

  std::vector<std::pair<void *, std::size_t>> allocations;

  std::geometric_distribution<std::size_t> allocation_bin_distribution;
  std::geometric_distribution<std::size_t> large_allocation_distribution;

public:
  std::size_t maximum_concurrent_allocations = 0;
  std::uint64_t allocation_count = 0;
  std::uint64_t deallocation_count = 0;

  RandomTest(std::uint64_t seed = 0x31337)
      : rng(seed)
#if defined(USE_KDALLOC)
        ,
        allocator(klee::kdalloc::AllocatorFactory(
            static_cast<std::size_t>(1) << 42, 0))
#endif
        ,
        allocation_bin_distribution(0.3),
        large_allocation_distribution(0.00003) {
  }

  void run(std::uint64_t const iterations) {
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
      auto choice = std::uniform_int_distribution<std::size_t>(
          0, allocations.size() - 1)(rng);
#if defined(USE_KDALLOC)
      allocator.free(allocations[choice].first, allocations[choice].second);
#else
      free(allocations[choice].first);
#endif
      allocations[choice] = allocations.back();
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

#if defined(USE_KDALLOC)
    allocations.emplace_back(allocator.allocate(size), size);
#else
    allocations.emplace_back(std::malloc(size), size);
#endif
    if (allocations.size() > maximum_concurrent_allocations) {
      maximum_concurrent_allocations = allocations.size();
    }
  }

  void allocate_large() {
    auto size = 0;
    while (size <= 4096 || size > 1073741825) {
      size = large_allocation_distribution(rng) + 4097;
    }

#if defined(USE_KDALLOC)
    allocations.emplace_back(allocator.allocate(size), size);
#else
    allocations.emplace_back(std::malloc(size), size);
#endif
    if (allocations.size() > maximum_concurrent_allocations) {
      maximum_concurrent_allocations = allocations.size();
    }
  }

  void deallocate() {
    if (allocations.empty()) {
      return;
    }
    auto choice = std::uniform_int_distribution<std::size_t>(
        0, allocations.size() - 1)(rng);
#if defined(USE_KDALLOC)
    allocator.free(allocations[choice].first, allocations[choice].second);
#else
    free(allocations[choice].first);
#endif
    allocations[choice] = allocations.back();
    allocations.pop_back();
  }
};
} // namespace

#if defined(USE_GTEST_INSTEAD_OF_MAIN)
int random_test() {
#else
int main() {
#endif
#if defined(USE_KDALLOC)
  std::cout << "Using kdalloc\n";
#else
  std::cout << "Using std::malloc\n";
#endif
  auto start = std::chrono::steady_clock::now();

  RandomTest tester;
  tester.run(10'000'000);

  auto stop = std::chrono::steady_clock::now();
  std::cout << std::dec
            << std::chrono::duration_cast<std::chrono::milliseconds>(stop -
                                                                     start)
                   .count()
            << " ms\n";
  std::cout << "\n";

  std::cout << "Allocations: " << tester.allocation_count << "\n";
  std::cout << "Deallocations: " << tester.deallocation_count << "\n";
  std::cout << "Maximum concurrent allocations: "
            << tester.maximum_concurrent_allocations << "\n";

  exit(0);
}

#if defined(USE_GTEST_INSTEAD_OF_MAIN)
TEST(KDAllocDeathTest, Random) {
  ASSERT_EXIT(random_test(), ::testing::ExitedWithCode(0), "");
}
#endif
