// RUN: %clang -emit-llvm -c -g %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=SeedGen %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: not test -f %t.klee-out/test000002.ktest

// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --seed-file %t.klee-out/test000001.ktest %t.bc | FileCheck --allow-empty %s
// RUN: %klee-stats --print-columns 'SolverQueries' --table-format=csv %t.klee-out-2 | FileCheck --check-prefix=CHECK-STATS %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void SeedGen() {
  size_t x;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x == 100);
}

int main(int argc, char **argv) {
  size_t s;
  klee_make_symbolic(&s, sizeof(s), "size");
  char *p = (char *)malloc(s);
  if (!p)
    return 0;

  if (s != 100)
    printf("Error\n");
    // CHECK-NOT: Error

  // CHECK-STATS: 4
  // These queries are due to the explicit constraint asserting that s is 100 and the implicit one checking if we have a huge malloc
}
