//===-- reuse.cpp ---------------------------------------------------------===//
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
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <unordered_set>

void reuse_test() {
  {
    static const std::size_t size = 8;
    static const std::uint32_t quarantine = 0;
    std::cout << "size = " << size << " quarantine = " << quarantine << "\n";
    auto allocator =
        static_cast<klee::kdalloc::Allocator>(klee::kdalloc::AllocatorFactory(
            static_cast<std::size_t>(1) << 42, quarantine));
    auto a = allocator.allocate(size);
    allocator.free(a, size);
    auto b = allocator.allocate(size);
    allocator.free(b, size);
    auto c = allocator.allocate(size);
    allocator.free(c, size);
    auto d = allocator.allocate(size);
    allocator.free(d, size);

    std::cout << "a: " << a << "\nb: " << b << "\nc: " << c << "\nd: " << d
              << "\n";
    assert(a == b);
    assert(a == c);
    assert(a == d);
  }

  std::cout << "\n\n";

  {
    static const std::size_t size = 8;
    static const std::uint32_t quarantine = 1;
    std::cout << "size = " << size << " quarantine = " << quarantine << "\n";
    auto allocator =
        static_cast<klee::kdalloc::Allocator>(klee::kdalloc::AllocatorFactory(
            static_cast<std::size_t>(1) << 42, quarantine));
    auto a = allocator.allocate(size);
    allocator.free(a, size);
    auto b = allocator.allocate(size);
    allocator.free(b, size);
    auto c = allocator.allocate(size);
    allocator.free(c, size);
    auto d = allocator.allocate(size);
    allocator.free(d, size);

    std::cout << "a: " << a << "\nb: " << b << "\nc: " << c << "\nd: " << d
              << "\n";
    assert(a != b);
    assert(a == c);
    assert(b == d);
  }

  std::cout << "\n\n";

  {
    static const std::size_t size = 8;
    static const std::uint32_t quarantine = 2;
    std::cout << "size = " << size << " quarantine = " << quarantine << "\n";
    auto allocator =
        static_cast<klee::kdalloc::Allocator>(klee::kdalloc::AllocatorFactory(
            static_cast<std::size_t>(1) << 42, quarantine));
    auto a = allocator.allocate(size);
    allocator.free(a, size);
    auto b = allocator.allocate(size);
    allocator.free(b, size);
    auto c = allocator.allocate(size);
    allocator.free(c, size);
    auto d = allocator.allocate(size);
    allocator.free(d, size);

    std::cout << "a: " << a << "\nb: " << b << "\nc: " << c << "\nd: " << d
              << "\n";
    assert(a != b);
    assert(a != c);
    assert(b != c);
    assert(a == d);
  }

  std::cout << "\n\n";

  {
    static const std::size_t size = 8;
    static const std::uint32_t quarantine =
        klee::kdalloc::AllocatorFactory::unlimitedQuarantine;
    std::cout << "size = " << size << " quarantine unlimited\n";
    auto allocator =
        static_cast<klee::kdalloc::Allocator>(klee::kdalloc::AllocatorFactory(
            static_cast<std::size_t>(1) << 42, quarantine));

#if KDALLOC_TRACE >= 2
    static const std::size_t iterations = 10;
#else
    static const std::size_t iterations = 10'000;
#endif
    std::unordered_set<void *> allocations;
    allocations.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
      auto *ptr = allocator.allocate(size);
      allocator.free(ptr, 8);
      auto emplaced = allocations.emplace(ptr);
      assert(emplaced.second);
    }
  }

  std::exit(0);
}

#if defined(USE_GTEST_INSTEAD_OF_MAIN)
TEST(KDAllocDeathTest, Reuse) {
  ASSERT_EXIT(reuse_test(), ::testing::ExitedWithCode(0), "");
}
#else
int main() { reuse_test(); }
#endif
