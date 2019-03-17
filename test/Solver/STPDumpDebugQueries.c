// REQUIRES: stp
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -solver-backend=stp --output-dir=%t.klee-out -debug-dump-stp-queries %t1.bc
// RUN: cat %t.klee-out/warnings.txt | FileCheck %s

// Objective: test -debug-dump-stp-queries (just invocation and header in warnings.txt)

#include "klee/klee.h"

int main(int argc, char **argv) {
  unsigned i;
  klee_make_symbolic(&i, sizeof i, "i");
  if (i)
    return 0;
  else
    return 1;
  // CHECK: KLEE: WARNING: STP query:
}
