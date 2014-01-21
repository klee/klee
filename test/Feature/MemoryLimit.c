// RUN: %llvmgcc -emit-llvm -DLITTLE_ALLOC -g -c %s -o %t.little.bc
// RUN: %llvmgcc -emit-llvm -g -c %s -o %t.big.bc
// RUN: %klee --max-memory=20 %t.little.bc > %t.little.log
// RUN: %klee --max-memory=20 %t.big.bc > %t.big.log
// RUN: not grep -q "MALLOC FAILED" %t.little.log
// RUN: not grep -q "MALLOC FAILED" %t.big.log
// RUN: not grep -q "DONE" %t.little.log
// RUN: not grep -q "DONE" %t.big.log

#include <stdlib.h>
#include <stdio.h>

int main() {
  int i, j, x=0, malloc_failed = 0;
  
#ifdef LITTLE_ALLOC
  printf("IN LITTLE ALLOC\n");
    
  // 200 MBs total (in 32 byte chunks)
  for (i=0; i<100 && !malloc_failed; i++) {
    for (j=0; j<(1<<16); j++){
      void * p = malloc(1<<5);
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
