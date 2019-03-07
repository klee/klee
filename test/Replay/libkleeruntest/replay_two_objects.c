// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs %t.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test ! -f %t.klee-out/test000002.ktest

// Now try to replay with libkleeRuntest
// RUN: %cc -DPRINT_VALUES %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner | FileCheck -check-prefix=TESTONE %s

#include "klee/klee.h"
#include <stdio.h>

int main(int argc, char** argv) {
  int x = 0;
  int y = 0;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), NULL);

  klee_assume(x == 1);
  klee_assume(y == 128);

#ifdef PRINT_VALUES
  printf("x=%d\n", x);
  printf("y=%d\n", y);
#endif

  return 0;
}
// CHECK: KLEE: done: completed paths = 1

// TESTONE: x=1
// TESTONE: y=128
