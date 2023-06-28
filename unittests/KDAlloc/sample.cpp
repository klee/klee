//===-- sample.cpp --------------------------------------------------------===//
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

void sample_test() {
  // initialize a factory and an associated allocator (1 TiB and no quarantine)
  klee::kdalloc::AllocatorFactory factory(static_cast<std::size_t>(1) << 40, 0);
  klee::kdalloc::Allocator allocator = factory.makeAllocator();

  // let us create an integer
  void *ptr = allocator.allocate(sizeof(int));
  int *my_int = static_cast<int *>(ptr);
  *my_int = 42;
  assert(*my_int == 42 &&
         "While we can use the addresses, this is not the point of kdalloc");

  {
    auto allocator2 = factory.makeAllocator();
    int *my_second_int = static_cast<int *>(allocator2.allocate(sizeof(int)));
    assert(reinterpret_cast<std::uintptr_t>(my_int) ==
               reinterpret_cast<std::uintptr_t>(my_second_int) &&
           "kdalloc is intended to produce reproducible addresses");
    allocator2.free(my_second_int, sizeof(int));
    assert(*my_int == 42 &&
           "The original allocation (from allocator) is still valid");
  }

  // now let us clone the allocator state
  {
    auto allocator2 = allocator;
    int *my_second_int = static_cast<int *>(allocator2.allocate(sizeof(int)));
    assert(reinterpret_cast<std::uintptr_t>(my_int) !=
               reinterpret_cast<std::uintptr_t>(my_second_int) &&
           "the new address must be different, as allocator2 also contains the "
           "previous allocation");
    allocator2.free(my_second_int, sizeof(int));
    assert(*my_int == 42 &&
           "The original allocation (from allocator) is still valid");
  }

  // there is no need to return allocated memory, so we omit
  // `allocator.free(my_int, sizeof(int));`
  exit(0);
}

#if defined(USE_GTEST_INSTEAD_OF_MAIN)
TEST(KDAllocDeathTest, Sample) {
  ASSERT_EXIT(sample_test(), ::testing::ExitedWithCode(0), "");
}
#else
int main() { sample_test(); }
#endif
