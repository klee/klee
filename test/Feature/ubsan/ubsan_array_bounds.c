// RUN: %clang %s -fsanitize=array-bounds -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 2
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 1

#include "klee/klee.h"

unsigned int array_index(unsigned int n) {
  unsigned int a[4] = {0};

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: out-of-bounds-index
  return a[n];
}

int main() {
  unsigned int x;
  volatile unsigned int result;

  klee_make_symbolic(&x, sizeof(x), "x");

  result = array_index(x);

  return 0;
}
