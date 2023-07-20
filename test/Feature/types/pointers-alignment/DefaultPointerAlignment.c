// RUN: %clang %s -emit-llvm -g -c -fsanitize=alignment,null -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --ubsan-runtime --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --align-symbolic-pointers=true --skip-local=false --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>

int main() {
  float value;

  float *ptr;
  klee_make_symbolic(&ptr, sizeof(ptr), "ptr");

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: null-pointer-use
  *ptr = 10;

  int n = klee_range(1, 4, "n");
  ptr = (float *)(((char *)ptr) + n);

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: misaligned-pointer-use
  *ptr = 20;

  return 0;
}
