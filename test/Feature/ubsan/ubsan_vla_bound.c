// REQUIRES: not-msan
// Disabling msan because it crash during posix_memalign call on CI
// RUN: %clang %s -fsanitize=vla-bound -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  int x;
  klee_make_symbolic(&x, sizeof x, "x");
  // CHECK: ubsan_vla_bound.c:[[@LINE+1]]: variable length array bound evaluates to non-positive value
  int arr[x];
  volatile int result = arr[0];
  return 0;
}
