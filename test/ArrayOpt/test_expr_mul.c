// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --use-query-log=all:kquery --output-dir=%t.klee-out %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK
// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize-array=value --write-kqueries --use-query-log=all:kquery --output-dir=%t.klee-out %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK-V

#include "klee/klee.h"
#include <stdio.h>

char array[10] = {1, 2, 3, 2, 5, 6, 5, 8, 9, 10};
char array2[10] = {0, 1, 7, 19, 20, 21, 22, 23, 24, 25};

int main() {
  char k;
  // CHECK-V: KLEE: WARNING: OPT_V: successful

  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < 4);
  klee_assume(k >= 1);

  // CHECK-DAG: Yes
  // CHECK-DAG: Yes
  char r = (array[k] * 3) & 1;
  char v = array2[r];
  if (v == 0)
    printf("Yes\n");
  else if (v == 1)
    printf("Yes\n");
  else
    printf("No\n");

  // CHECK-DAG: KLEE: done: completed paths = 2
  // CHECK-NOT: No

  return 0;
}
