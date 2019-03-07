// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=index %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK-OPT_I
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: test -f %t.klee-out/test000002.kquery
// RUN: test -f %t.klee-out/test000003.kquery
// RUN: test -f %t.klee-out/test000004.kquery
// RUN: test -f %t.klee-out/test000005.kquery
// RUN: test -f %t.klee-out/test000006.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000002.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000003.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000004.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000005.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000006.kquery -check-prefix=CHECK-CONST_ARR
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=value %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK-OPT_V
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: test -f %t.klee-out/test000002.kquery
// RUN: test -f %t.klee-out/test000003.kquery
// RUN: test -f %t.klee-out/test000004.kquery
// RUN: test -f %t.klee-out/test000005.kquery
// RUN: test -f %t.klee-out/test000006.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000002.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000003.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000004.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000005.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000006.kquery -check-prefix=CHECK-CONST_ARR
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize-array=all %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log -check-prefix=CHECK-OPT_I
// RUN: test -f %t.klee-out/test000001.kquery
// RUN: test -f %t.klee-out/test000002.kquery
// RUN: test -f %t.klee-out/test000003.kquery
// RUN: test -f %t.klee-out/test000004.kquery
// RUN: test -f %t.klee-out/test000005.kquery
// RUN: test -f %t.klee-out/test000006.kquery
// RUN: not FileCheck %s -input-file=%t.klee-out/test000001.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000002.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000003.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000004.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000005.kquery -check-prefix=CHECK-CONST_ARR
// RUN: not FileCheck %s -input-file=%t.klee-out/test000006.kquery -check-prefix=CHECK-CONST_ARR

// CHECK-OPT_I: KLEE: WARNING: OPT_I: successful
// CHECK-OPT_V: KLEE: WARNING: OPT_V: successful
// CHECK-CONST_ARR: const_arr

#include <stdio.h>
#include "klee/klee.h"

char array[5] = {1,2,3,4,5};
char arrayconc[4] = {2,4,6,8};
char arraychar[2] = {'a',127};

int main() {
  char k, x;

  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < 5);
  klee_assume(k >= 0);
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x < 2);
  klee_assume(x >= 0);

  // CHECK: Yes
  // CHECK: No
  // CHECK: Good
  // CHECK: Char
  // CHECK: Concrete
  if (array[k] == 3)
    printf("Yes\n");
  else if (array[k] > 4)
    printf("No\n");
  else if (array[k] + k - 2 == 6)
    printf("Good\n");

  if (arraychar[x] > 126)
    printf("Char\n");

  int i = array[4] * 3 - 12;
  if (arrayconc[i] > 3) {
    printf("Concrete\n");
  }

  // CHECK: KLEE: done: completed paths = 6

  return 0;
}
