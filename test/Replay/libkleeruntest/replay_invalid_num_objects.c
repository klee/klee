// Compile program that only makes one klee_make_symbolic() call
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs %t.bc
// RUN: test -f %t.klee-out/test000001.ktest

// Now try to replay with libkleeRuntest but build the binary so it
// makes two calls to klee_make_symbolic.
// RUN: %cc -DEXTRA_MAKE_SYMBOLIC %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner

// Check that the default is to exit with an error
// RUN: not env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_FATAL %s

// Check that setting `KLEE_RUN_TEST_ERRORS_NON_FATAL` will not exit with an error
// and will continue executing.
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest KLEE_RUN_TEST_ERRORS_NON_FATAL=1 %t_runner 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>

int main(int argc, char** argv) {
  int x = 0;
  klee_make_symbolic(&x, sizeof(x), "x");

#ifdef EXTRA_MAKE_SYMBOLIC
  int y = 1;
  klee_make_symbolic(&y, sizeof(y), "x");
  klee_assume(y == 0);
  fprintf(stderr, "y is \"%d\"\n", y);
#endif
  return 0;
}
// CHECK: KLEE_RUN_TEST_ERROR: out of inputs
// CHECK: KLEE_RUN_TEST_ERROR: Forcing execution to continue
// CHECK: y is "0"

// CHECK_FATAL: KLEE_RUN_TEST_ERROR: out of inputs
// CHECK_FATAL-NOT: y is "0"

