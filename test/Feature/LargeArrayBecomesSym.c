/* This test checks that KLEE emits a warning when a large (> 4096) 
   concrete array becomes symbolic. */

// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
/* The solver timeout is needed as some solvers, such as metaSMT+CVC4, time out here. */
// RUN: %klee --max-solver-time=2 --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>

#define N 4100

int main() {
  char a[N] = {
      1,
      2,
  };

  unsigned k;
  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < N);
  a[k] = 3;
  // CHECK: KLEE: WARNING ONCE: Symbolic memory access will send the following array of 4100 bytes to the constraint solver

  unsigned i;
  klee_make_symbolic(&i, sizeof(i), "i");
  klee_assume(i < N);
  if (a[i] == 2)
    assert(i == 1);
}
