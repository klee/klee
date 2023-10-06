// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --skip-not-symbolic-objects %t.bc > %t.log
// RUN: FileCheck %s -input-file=%t.log
#include "klee/klee.h"

int main() {
  int *a;
  klee_make_symbolic(&a, sizeof(a), "a");
  int b = *a;

  // CHECK-DAG: Yes
  // CHECK-DAG: No
  if (b > 0) {
    printf("Yes\n");
  } else {
    printf("No\n");
  }
  return 0;
}
