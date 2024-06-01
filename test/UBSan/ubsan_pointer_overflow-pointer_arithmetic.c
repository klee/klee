// RUN: %clang %s -fsanitize=pointer-overflow -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 2
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 1

#include "klee/klee.h"
#include <stdio.h>

int main() {
  char c;
  char* ptr = &c;

  size_t offset;
  volatile char* result;

  klee_make_symbolic(&offset, sizeof(offset), "offset");
  klee_assume((size_t)(ptr) + offset != 0);

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: pointer-overflow
  result = ptr + offset;

  return 0;
}
