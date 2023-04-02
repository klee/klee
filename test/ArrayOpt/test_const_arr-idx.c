// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=index %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log
// RUN: not FileCheck %s -input-file=%t.log -check-prefix=CHECK-OPT_I
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=value %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log
// RUN: not FileCheck %s -input-file=%t.log -check-prefix=CHECK-OPT_V
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=all %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log
// RUN: not FileCheck %s -input-file=%t.log -check-prefix=CHECK-OPT_I
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR

// CHECK-OPT_I: KLEE: WARNING: OPT_I: successful
// CHECK-OPT_V: KLEE: WARNING: OPT_V: successful
// CHECK-CONST_ARR: const_arr

#include <stdio.h>
#include "klee/klee.h"

unsigned array[5] = {1,2,3,4,5};
int arr[3] = {0,1,2};

int main() {
  int x = 2;
  unsigned k = arr[x];

  // CHECK-DAG: Yes
  if (array[k] == 3)
    printf("Yes\n");

  // CHECK-DAG: KLEE: done: completed paths = 1

  return 0;
}
