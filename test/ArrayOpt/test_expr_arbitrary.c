// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --use-query-log=all:kquery --output-dir=%t.klee-out %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK
// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize-array=value --write-kqueries --use-query-log=all:kquery --output-dir=%t.klee-out %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK-V

#include "klee/klee.h"
#include <stdio.h>

short array[10] = {42, 1, 42, 42, 2, 5, 6, 42, 8, 9};

int main() {
  char k;
  // CHECK-V: KLEE: WARNING: OPT_V: successful

  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < 4);
  klee_assume(k >= 0);

  short *ptrs[4] = {array + 3, array + 0, array + 7, array + 2};

  // CHECK-DAG: Yes
  if ((*(ptrs[k])) == 42)
    printf("Yes\n");
  else
    printf("No\n");

  // CHECK-DAG: KLEE: done: completed paths = 1
  // CHECK-NOT: No

  return 0;
}
