// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --warnings-only-to-file=false %t1.bc 2>&1 | FileCheck %s

// REQUIRES: not-freebsd

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "klee/klee.h"

int main() {
  size_t sz = (size_t) 1 + ((size_t) 1 << 32);
  char *p = malloc(sz);
  // CHECK: WARNING ONCE: Large memory allocation (4294967297 bytes). KLEE may run out of memory.
  assert(p);
  p[sz - 1] = 42;

  if (klee_choose(2)) {
    assert(p[sz - 1] == 42);
    printf("Success\n");
    // CHECK-DAG: Success
  } else
    klee_make_symbolic(p, sz, "p");
    // CHECK-DAG: Symbolic objects larger than 4 GiB are not allowed (requested 4294967297 bytes).
}
