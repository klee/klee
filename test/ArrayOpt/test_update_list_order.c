// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize-array=value --use-query-log=all:kquery --write-kqueries --output-dir=%t.klee-out %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log

// CHECK-DAG: KLEE: WARNING: OPT_V: successful

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

char array[5] = {1, 2, 3, 4, 5};

int main() {
  char idx;
  char updateListBreaker;
  klee_make_symbolic(&updateListBreaker, sizeof(updateListBreaker), "ulBreak");
  klee_make_symbolic(&idx, sizeof(idx), "idx");
  klee_assume(idx < 5);
  klee_assume(idx > 0);
  klee_assume(updateListBreaker != 3);
  array[0] = updateListBreaker;
  array[2] = 6;
  updateListBreaker = array[idx];
  array[2] = 3;
  assert(updateListBreaker != 3 && "keep updateListBreak alive");

  // CHECK-DAG: KLEE: done: completed paths = 2
  // CHECK-DAG: No
  // CHECK-DAG: Yes
  if (array[idx] == 3)
    printf("Yes\n");
  else
    printf("No\n");

  return 0;
}
