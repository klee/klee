// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize-array=value --use-query-log=all:kquery --write-kqueries --output-dir=%t.klee-out %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK-OPT_V
// RUN: FileCheck %s -input-file=%t.log

// CHECK-OPT_V: KLEE: WARNING: OPT_V: successful
// CHECK-CONST_ARR: const_arr

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

short array[6] = {1, 2, 3, 4, 5, 0};

int main() {
  char idx;
  short symHole, symHole2;
  klee_make_symbolic(&symHole, sizeof(symHole), "hole");
  klee_make_symbolic(&symHole2, sizeof(symHole2), "hole2");
  klee_make_symbolic(&idx, sizeof(idx), "idx");
  klee_assume(idx < 5);
  klee_assume(idx > 0);
  klee_assume((symHole < 401) & (symHole > 399));
  klee_assume(symHole2 != 400);
  array[2] = symHole;
  array[0] = symHole2;

  // CHECK: KLEE: done: completed paths = 2
  // CHECK: Yes
  // CHECK-NEXT: No
  if (array[idx + 1] == 400)
    printf("Yes\n");
  else
    printf("No\n");

  return 0;
}
