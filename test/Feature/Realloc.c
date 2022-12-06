// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --warnings-only-to-file=false %t1.bc 2>&1 | FileCheck %s

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int *p = malloc(sizeof(int) * 2);
  assert(p);
  p[1] = 52;

  int *p2 = realloc(p, 1 << 30);
  // CHECK-NOT: ASSERTION FAIL
  assert(!p2 || p2[1] == 52);

  return 0;
}
