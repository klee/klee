// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out %t.klee-out1
// RUN: %klee --output-dir=%t.klee-out --search=dfs -pending %t.bc > %t.out 2> %t.err
// RUN: FileCheck -input-file=%t.err %s
// RUN: FileCheck -input-file=%t.out -check-prefix=PCHECK %s
// RUN: %klee -max-forks=1 --output-dir=%t.klee-out1 --search=dfs -pending %t.bc > %t.out 2> %t.err
// RUN: FileCheck -input-file=%t.err -check-prefix=FORKSKIP %s

#include "klee/klee.h"

#include <stdio.h>

// CHECK-DAG: KLEE: done: completed paths = 3
// CHECK-DAG: KLEE: done: generated tests = 3
// FORKSKIP: skipping fork (max-forks reached)
int run(unsigned char *x) {
  if (x[2] >= 10) {
    return 1;
  }

  if (x[2] == 8) {
    return 2;
  } else if (x[2] == 14) {
    return 5;
  } else {
    return 3;
  }
}

unsigned char y[255];

int main() {
  unsigned char x[4];
  klee_make_symbolic(&x, sizeof x, "x");

  // PCHECK-DAG: Path 1
  // PCHECK-DAG: Path 2
  // PCHECK-DAG: Path 3
  printf("Path %d\n", run(x));
  return 0;
}
