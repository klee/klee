// RUN: %clang %s -emit-llvm -g -c -fsanitize=alignment,null -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --type-system=CXX --use-tbaa --use-lazy-initialization=false --align-symbolic-pointers=true --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>

int main() {
  float value;

  float *ptr;
  klee_make_symbolic(&ptr, sizeof(ptr), "ptr");

  // CHECK-NOT: DefaultPointerAlignment.c:[[@LINE+1]]: either misaligned address for 0x{{.*}} or invalid usage of address 0x{{.*}} with insufficient space
  *ptr = 10;

  int n = klee_range(1, 4, "n");
  ptr = (float *)(((char *)ptr) + n);

  // CHECK: DefaultPointerAlignment.c:[[@LINE+1]]: either misaligned address for 0x{{.*}} or invalid usage of address 0x{{.*}} with insufficient space
  *ptr = 20;

  return 0;
}
