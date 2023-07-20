// RUN: %clang %s -emit-llvm -g -c -fsanitize=alignment,null -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-advanced-type-system --use-tbaa --use-lazy-initialization=none --align-symbolic-pointers=true --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>

int main() {
  int16_t value = 0;

  int16_t *array[2];
  klee_make_symbolic(&array, sizeof(array), "array");

  // CHECK: x
  if (!array[1]) {
    printf("x");
    return 0;
  }

  // CHECK-NOT: ArrayPointerAlignment.c:[[@LINE+1]]: either misaligned address 0x{{.*}} or invalid usage of address 0x{{.*}} with insufficient space
  *(array[1]) = 100;
  return 0;
}
