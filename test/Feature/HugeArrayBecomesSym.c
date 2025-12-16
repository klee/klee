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
  p[1] = 42;

  unsigned k;
  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < 100);
  if (p[k] == 3)
    printf("3");
  // CHECK: Symbolic reads from objects larger than 4 GiB are not allowed (object size: 4294967297 bytes)
}
