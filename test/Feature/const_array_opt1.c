// REQUIRES: not-msan
// REQUIRES: not-ubsan
// Disabling msan and ubsan because it times out on CI
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --const-array-opt --max-time=10 --only-output-states-covering-new %t.bc | FileCheck %s

/* This is testing the const array optimization.  On my 2.3GHz machine
   this takes under 2 seconds w/ the optimization and almost 6 minutes
   w/o.  So we kill it in 10 sec and check if it has finished
   successfully. */
#include "klee/klee.h"

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

  // CHECK: Finished successfully!
  printf("Finished successfully!\n");

  return 0;
}
