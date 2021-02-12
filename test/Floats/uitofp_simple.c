// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

int main() {
  float x;
  uint32_t y;
  klee_make_symbolic(&y, sizeof(uint32_t), "y");
  x = (float) y;
  assert(klee_is_symbolic(x));
  assert( x >= 0.0f);
  assert( x <= 0x1.000000p32); // x <= 2^32
  return 0;
}
// CHECK-NOT: silently concretizing (reason: floating point)
// CHECK: KLEE: done: completed paths = 1
