// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=index %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK -check-prefix=CHECK-OPT_I
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: test -f %t.klee-out/test000002.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000002.kquery -check-prefix=CHECK-CONST_ARR
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=value %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK -check-prefix=CHECK-OPT_V
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: test -f %t.klee-out/test000002.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000002.kquery -check-prefix=CHECK-CONST_ARR
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=all   %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK -check-prefix=CHECK-OPT_I
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: test -f %t.klee-out/test000002.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000002.kquery -check-prefix=CHECK-CONST_ARR

// CHECK-OPT_I: KLEE: WARNING: OPT_I: successful
// CHECK-OPT_V: KLEE: WARNING: OPT_V: successful
// CHECK-CONST_ARR: const_arr

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

char array[5] = {0,1,0,1,0};

int main() {
  unsigned k;

  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < 5);

  // CHECK-DAG: zero
  // CHECK-DAG: Correct!
  if (array[k] == 0) {
    printf("zero\n");
    if (k==0|k==2|k==4) {
      printf("Correct!\n");
    } else {
      klee_assert(0);
    }
  }

  // CHECK-DAG: KLEE: done: completed paths = 2

  return 0;
}
