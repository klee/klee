// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --size-model=range --capacity=10 %t.bc
// RUN: %cc %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner 2>&1

#include <stdio.h>
#include <stdlib.h>

#include "klee/klee.h"

int main(int argc, char** argv) {
  unsigned n;
  klee_make_symbolic(&n, sizeof(n), "n");
  char *s = malloc(n);

  return 0;
}
