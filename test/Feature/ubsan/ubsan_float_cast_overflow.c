// RUN: %clang %s -fsanitize=float-cast-overflow -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  float f = 0x7fffff80;
  volatile int result;

  //  klee_make_symbolic(&f, sizeof(f), "f");

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: float-cast-overflow
  result = f + 0x80;
  return 0;
}
