// REQUIRES: not-msan
// MSan adds additional memory that overflows the counter
//
// Check that we properly kill states when we exceed our memory bounds, for both
// small and large allocations (large allocations commonly use mmap(), which can
// follow a separate path in the allocator and statistics reporting).

// RUN: %clang -emit-llvm -DLITTLE_ALLOC -g -c %s -o %t.little.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-memory=20 %t.little.bc > %t.little.log
// RUN: FileCheck -check-prefix=CHECK-LITTLE -input-file=%t.little.log %s
// RUN: FileCheck -check-prefix=CHECK-WRN -input-file=%t.klee-out/warnings.txt %s

// RUN: %clang -emit-llvm -g -c %s -o %t.big.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-memory=20 %t.big.bc > %t.big.log 2> %t.big.err
// RUN: FileCheck -check-prefix=CHECK-BIG -input-file=%t.big.log %s
// RUN: FileCheck -check-prefix=CHECK-WRN -input-file=%t.klee-out/warnings.txt %s

#include <stdio.h>
#include <stdlib.h>

int main() {
  int i, j, x = 0, malloc_failed = 0;

#ifdef LITTLE_ALLOC
  printf("IN LITTLE ALLOC\n");
  // CHECK-LITTLE: IN LITTLE ALLOC

  // 200 MB total (in 1 KB chunks)
  for (i = 0; i < 100 && !malloc_failed; i++) {
    for (j = 0; j < (1 << 11); j++) {
      void *p = malloc(1 << 10);
      malloc_failed |= (p == 0);
    }
  }
#else
  printf("IN BIG ALLOC\n");
  // CHECK-BIG: IN BIG ALLOC

  // 200 MBs total
  for (i = 0; i < 100 && !malloc_failed; i++) {
    void *p = malloc(1 << 21);
    malloc_failed |= (p == 0);
    // Ensure we hit the periodic check
    // Use the pointer to be not optimized out by the compiler
    for (j = 0; j < 10000; j++)
      x += (long)p;
  }
#endif

// CHECK-WRN: WARNING: killing 1 states (over memory cap

  if (malloc_failed)
    printf("MALLOC FAILED\n");
    // CHECK-LITTLE-NOT: MALLOC FAILED
    // CHECK-BIG-NOT: MALLOC FAILED
  printf("DONE!\n");
  // CHECK-LITTLE-NOT: DONE!
  // CHECK-BIG-NOT: DONE!

  return x;
}
