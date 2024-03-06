// Something is wrong in the replaying process, compiled binary evalaluates fmin of nan and number as nan
// XFAIL: not-bitwuzla
// REQUIRES: floating-point
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --use-cex-cache=false --output-dir=%t.klee-out --search=dfs %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest

// Now try to replay with libkleeRuntest
// RUN: %cc %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -lm -o %t_runner
// RUN: rm -f %t_runner.log
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner >> %t_runner.log
// RUN: env KTEST_FILE=%t.klee-out/test000002.ktest %t_runner >> %t_runner.log
// RUN: FileCheck %s -check-prefix=CHECK -input-file=%t_runner.log

#include "klee/klee.h"
#include <math.h>
#include <stdio.h>

int main() {
  double a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");
  if (fmin(a, b) == a) {
    printf("fmin(a, b) == a\n");
  } else {
    printf("fmin(a, b) != a\n");
  }
  return 0;
}

// CHECK-DAG: fmin(a, b) != a
// CHECK-DAG: fmin(a, b) == a
