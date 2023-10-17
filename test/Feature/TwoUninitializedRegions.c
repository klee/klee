// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  char arr1[3];
  char arr2[3];

  if (arr1[0] == arr2[0]) {
    printf("1) equal\n");
  } else {
    printf("1) not equal\n");
  }

  // CHECK-DAG: 1) equal
  // CHECK-DAG: 1) not equal
}
