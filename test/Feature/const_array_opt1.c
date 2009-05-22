// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: %klee --const-array-opt --max-time=10 --only-output-states-covering-new %t.bc >%t.log
// grep -q "Finished successfully!\n"

/* This is testing the const array optimization.  On my 2.3GHz machine
   this takes under 2 seconds w/ the optimization and almost 6 minutes
   w/o.  So we kill it in 10 sec and check if it has finished
   successfully. */

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
  
int main() {
#define N 8192
#define N_IDX 16
  unsigned char a[N];
  unsigned i, k[N_IDX];

  for (i=0; i<N; i++)
    a[i] = i % 256;
  
  klee_make_symbolic(k, sizeof(k), "k");
  
  for (i=0; i<N_IDX; i++) {
    if (k[i] >= N)
      klee_silent_exit(0);

    if (a[k[i]] == i)
      assert(k[i] % 256 == i);
    else klee_silent_exit(0);
  }

  printf("Finished successfully!\n");

  return 0;
}
