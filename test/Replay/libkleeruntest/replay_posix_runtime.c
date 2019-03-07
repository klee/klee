// REQUIRES: posix-runtime
// FIXME: We need to fix a bug in libkleeRuntest
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime --search=dfs %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest

// Now try to replay with libkleeRuntest
// RUN: %cc %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner
// RUN: %ktest-tool %t.klee-out/test000001.ktest | FileCheck -check-prefix=CHECKMODEL %s
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner | FileCheck -check-prefix=TESTONE %s
// RUN: env KTEST_FILE=%t.klee-out/test000002.ktest %t_runner | FileCheck -check-prefix=TESTTWO %s

#include "klee/klee.h"
#include <stdio.h>

int main(int argc, char** argv) {
  int x = 0;
  klee_make_symbolic(&x, sizeof(x), "x");

  if (x == 0) {
    printf("x is 0\n");
  } else {
    printf("x is not 0\n");
  }
  return 0;
}

// CHECKMODEL: num objects: 2
// CHECKMODEL: object 0: name: {{b*}}'model_version'
// CHECKMODEL: object 1: name: {{b*}}'x'

// TESTONE: x is not 0
// TESTTWO: x is 0
