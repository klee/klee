// RUN: %llvmgcc -DLITTLE_ALLOC -g -c %s -o %t.little.bc
// RUN: %llvmgcc -g -c %s -o %t.big.bc
// RUN: %klee --max-memory=20 %t.little.bc > %t.little.log
// RUN: %klee --max-memory=20 %t.big.bc > %t.big.log
// RUN: not grep -q "DONE" %t.little.log
// RUN: not grep -q "DONE" %t.big.log

#include <stdlib.h>

int main() {
  int i, j, x=0;
  
#ifdef LITTLE_ALLOC
  printf("IN LITTLE ALLOC\n");
    
  // 200 MBs total (in 32 byte chunks)
  for (i=0; i<100; i++) {
    for (j=0; j<(1<<16); j++)
      malloc(1<<5);
  }
#else
  printf("IN BIG ALLOC\n");
  
  // 200 MBs total
  for (i=0; i<100; i++) {
    malloc(1<<21);
    
    // Ensure we hit the periodic check
    for (j=0; j<10000; j++)
      x++;
  }
#endif

  printf("DONE!\n");

  return x;
}
