// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s
#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  char arr[3];

  if (arr[0] == 0) {
    printf("1) 0\n");
  } else {
    printf("1) not 0\n");
  }

  char *zero_arr = (char *)calloc(3, sizeof(char));
  if (zero_arr[1] == 0) {
    return 0;
  } else {
    // CHECK-NOT: ASSERTION FAIL
    assert(0);
  }

  // CHECK-DAG: 1) 0
  // CHECK-DAG: 1) not 0
}
