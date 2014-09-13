// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc 2>&1 | FileCheck %s

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
  int *p = malloc(sizeof(int)*2);
  assert(p);
  p[1] = 52;

  // CHECK: KLEE: WARNING ONCE: Large alloc
  int *p2 = realloc(p, 1<<30);
  assert(p2[1] == 52);

  return 0;
}
