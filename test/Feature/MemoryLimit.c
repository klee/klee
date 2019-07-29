// REQUIRES: not-msan
// Memsan adds additional memory that overflows the counter
// Check that we properly kill states when we exceed our memory bounds, for both
// small and large allocations (large allocations commonly use mmap(), which can
// follow a separate path in the allocator and statistics reporting).

// RUN: %clang -emit-llvm -DLITTLE_ALLOC -g -c %s -o %t.little.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-memory=20 %t.little.bc > %t.little.log
// RUN: not grep -q "MALLOC FAILED" %t.little.log
// RUN: not grep -q "DONE" %t.little.log
// RUN: grep "WARNING: killing 1 states (over memory cap)" %t.klee-out/warnings.txt

// RUN: %clang -emit-llvm -g -c %s -o %t.big.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-memory=20 %t.big.bc > %t.big.log 2> %t.big.err
// RUN: not grep -q "MALLOC FAILED" %t.big.log
// RUN: not grep -q "DONE" %t.big.log
// RUN: grep "WARNING: killing 1 states (over memory cap)" %t.klee-out/warnings.txt

#include <stdlib.h>
#include <stdio.h>

int main() {
  int i, j, x=0, malloc_failed = 0;
  
#ifdef LITTLE_ALLOC
  printf("IN LITTLE ALLOC\n");
    
  // 200 MBs total (in 1k chunks)
  for (i=0; i<100 && !malloc_failed; i++) {
    for (j=0; j<(1<<11); j++){
      void * p = malloc(1<<10);
      malloc_failed |= (p == 0);
    }
  }
#else
  printf("IN BIG ALLOC\n");
  
  // 200 MBs total
  for (i=0; i<100 && !malloc_failed; i++) {
    void *p = malloc(1<<21);
    malloc_failed |= (p == 0);
    // Ensure we hit the periodic check
    // Use the pointer to be not optimized out by the compiler
    for (j=0; j<10000; j++)
      x+=(unsigned)p;
  }
#endif

  if (malloc_failed)
    printf("MALLOC FAILED\n");
  printf("DONE!\n");

  return x;
}
