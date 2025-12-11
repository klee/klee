// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --warnings-only-to-file=false %t1.bc

// REQUIRES: not-freebsd

#include <assert.h>
#include <stdlib.h>

#include "klee/klee.h"

int main() {
  size_t sz = (size_t) 1 + ((size_t) 1 << 32);
  char *p = malloc(sz);
  assert(p);
  // If large sizes are not handled properly, an overflow is incorrectly reported here.
  p[sz -1] = 42;
  free(p);
}
