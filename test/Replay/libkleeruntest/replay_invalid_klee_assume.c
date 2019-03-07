// RUN: %clang -DASSUME_VALUE=1 %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test ! -f %t.klee-out/test000002.ktest

// Now try to replay with libkleeRuntest but build the binary to use a different
// value for the `klee_assume()` call.
// RUN: %cc -DASSUME_VALUE=32 -DPRINT_VALUE %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner

// Check that the default is to exit with an error
// RUN: not env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_FATAL %s

// Check that setting `KLEE_RUN_TEST_ERRORS_NON_FATAL` will not exit with an error
// and will continue executing.
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest KLEE_RUN_TEST_ERRORS_NON_FATAL=1 %t_runner 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdint.h>
#include <stdio.h>

#ifndef ASSUME_VALUE
#error ASSUME_VALUE must be defined
#endif


int main(int argc, char** argv) {
  int x = 54;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x == ASSUME_VALUE);

#ifdef PRINT_VALUE
  printf("x=%d\n", x);
#endif

  return 0;
}
// CHECK: KLEE_RUN_TEST_ERROR: invalid klee_assume
// CHECK: KLEE_RUN_TEST_ERROR: Forcing execution to continue
// CHECK: x=1

// CHECK_FATAL: KLEE_RUN_TEST_ERROR: invalid klee_assume
// CHECK_FATAL-NOT: x=1

