// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-out-of-memory --use-sym-size-alloc %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdlib.h>

int main() {
  int n = klee_int("n");

  char *s1 = (char *)malloc(n);         // 0
  char *s2 = (char *)malloc(n / 2);     // 0, 0
  char *s3 = (char *)malloc(n / 3 - 2); // 6, 3, 0

  // CHECK-NOT: MultipleAllocations.c:[[@LINE+1]]: memory error: out of bound pointer
  s1[1] = 29;
}
