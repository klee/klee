// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc
//
// There should be a unique solution
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test ! -f %t.klee-out/test000002.ktest

// Now try to replay with libkleeRuntest
// RUN: %cc %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>

int main(int argc, char** argv) {
  int x = 0;
  klee_make_symbolic(&x, sizeof(x), "x");

  if (klee_is_replay()) { // only print x on replay runs
    printf("x = %d\n", x);
  }

  // For the sake of testing, these constraints have a single solution
  klee_assume(40 < x);
  klee_assume(x < 43);
  klee_assume((x & 1) == 0);

  return 0;
}

// When replaying, it should print the concrete value it is running with
// CHECK: x = 42
