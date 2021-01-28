// RUN: %clangxx %s -fsanitize=return -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int no_return() {
  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: missing-return
}

int main() {
  volatile int result = no_return();
  return 0;
}
