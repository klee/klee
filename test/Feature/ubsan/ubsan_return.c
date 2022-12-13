// RUN: %clangxx %s -fsanitize=return -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int no_return() {
  // CHECK: ubsan_return.c:[[@LINE-1]]: execution reached the end of a value-returning function without returning a value
}

int main() {
  volatile int result = no_return();
  return 0;
}
