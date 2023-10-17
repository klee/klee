// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-sym-size-alloc --skip-not-symbolic-objects --use-merged-pointer-dereference=true %t.bc 2>&1 | FileCheck %s
#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  char *s1 = (char *)malloc(1);
  char *s2 = (char *)malloc(1);
  if (s1[0] == s2[0]) {
    printf("1) eq\n");
  } else {
    printf("1) not eq\n");
  }

  // CHECK-DAG: 1) eq
  // CHECK-DAG: 1) not eq
}
