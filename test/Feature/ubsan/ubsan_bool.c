// RUN: %clang %s -fsanitize=bool -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  unsigned char x;
  volatile _Bool result;

  klee_make_symbolic(&x, sizeof(x), "x");

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: load invalid value
  result = *(_Bool *)&x;

  return 0;
}
