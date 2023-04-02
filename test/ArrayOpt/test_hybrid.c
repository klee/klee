// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=index %t.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR

// CHECK: KLEE: WARNING: OPT_I: successful
// CHECK-CONST_ARR: const_arr

#include <stdio.h>
#include "klee/klee.h"

char array[10] = {1,2,2,4,5,3,1,3,2,2};

int main() {
  unsigned char k;
  
  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < 10);
  
  // CHECK-DAG: Yes
  if (array[k] >= 1)
    printf("Yes\n");

  // CHECK-DAG: KLEE: done: completed paths = 1
  
  return 0;
}
