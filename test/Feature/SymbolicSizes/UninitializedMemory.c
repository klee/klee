// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-sym-size-alloc --skip-not-symbolic-objects --use-merged-pointer-dereference=true %t.bc 2>&1 | FileCheck %s
#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int n;
  klee_make_symbolic(&n, sizeof(n), "n");

  char *s = (char *)malloc(n);
  s[2] = 10;
  if (s[0] == 0) {
    printf("1) 0\n");
  } else {
    printf("1) not 0\n");
  }

  // CHECK-DAG: 1) 0
  // CHECK-DAG: 1) not 0
}
