// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc

// This should produce three test cases.
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: test ! -f %t.klee-out/test000004.ktest

// Now try to replay with libkleeRuntest
// RUN: %cc -DPRINT_VALUE %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner

// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_1 %s
// RUN: env KTEST_FILE=%t.klee-out/test000002.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_2 %s
// RUN: env KTEST_FILE=%t.klee-out/test000003.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_3 %s

#include "klee/klee.h"
#include <stdint.h>
#include <stdio.h>

int main(int argc, char** argv) {
  int x = 54, y = 55;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), "y");
  klee_prefer_cex(&x, x == 33);

  if (x < 40) {
    if (y == 0) {
      klee_assume(x == 0);
      x++;
      // It's fine if the prefered value cannot be used
      // CHECK_3: x=1, y=0
    } else {
      printf("x is allowed to be 33\n");
      // The prefered value should be used if it can be
      // CHECK_2: x=33
    }
  } else {
    printf("x is not allowed to be 33\n");
    // CHECK_1-NOT: x=33
  }

#ifdef PRINT_VALUE
  printf("x=%d, y=%d\n", x, y);
#endif

  return 0;
}
