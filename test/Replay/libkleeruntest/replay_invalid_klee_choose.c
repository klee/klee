// RUN: %clang -DBOUND_VALUE=32 -DFORCE_VALUE=20 %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test ! -f %t.klee-out/test000002.ktest

// Now try to replay with libkleeRuntest but build the binary to use a different
// bound for `klee_choose()`.
// RUN: %cc -DBOUND_VALUE=2 -DPRINT_VALUE %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner

// Check that the default is to exit with an error
// RUN: not env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_FATAL %s

// Check that setting `KLEE_RUN_TEST_ERRORS_NON_FATAL` will not exit with an error
// and will continue executing.
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest KLEE_RUN_TEST_ERRORS_NON_FATAL=1 %t_runner 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdint.h>
#include <stdio.h>

#ifndef BOUND_VALUE
#error BOUND_VALUE must be defined
#endif


int main(int argc, char** argv) {
  int x = klee_choose(BOUND_VALUE);
#ifdef FORCE_VALUE
  klee_assume(x == FORCE_VALUE);
#endif

#ifdef PRINT_VALUE
  printf("x=%d\n", x);
#endif

  return 0;
}
// CHECK: KLEE_RUN_TEST_ERROR: klee_choose failure. max = 2, got = 20
// CHECK: KLEE_RUN_TEST_ERROR: Forcing execution to continue
// CHECK: x=20

// CHECK_FATAL: KLEE_RUN_TEST_ERROR: klee_choose failure. max = 2, got = 20
// CHECK_FATAL-NOT: x=20

