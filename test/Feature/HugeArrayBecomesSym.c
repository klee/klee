// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --warnings-only-to-file=false --max-sym-obj-size=32 %t1.bc 2>&1 | FileCheck %s

// REQUIRES: not-freebsd

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "klee/klee.h"

int main() {
  size_t sz = 33;
  char *p = malloc(sz);
  assert(p);
  p[1] = 42;

  unsigned k;
  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < sz);
  if (p[k] == 3)
    printf("3");
  // CHECK: Symbolic reads from objects larger than 32 bytes are not allowed (object size: 33 bytes)
}
