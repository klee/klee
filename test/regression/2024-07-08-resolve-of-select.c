// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --skip-not-symbolic-objects --skip-not-lazy-initialized --skip-local --skip-global %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>

int main() {
  int x = 0;
  int y = 1;

  int cond;
  klee_make_symbolic(&cond, sizeof(cond), "cond");

  int *z = cond ? &x : &y;
  // CHECK-NOT: memory error: out of bound pointer
  *z = 10;

  // CHECK-NOT: ASSERTION FAIL
  if (x == 10) {
    assert(y == 1);
  } else {
    assert(x == 0);
    assert(y == 10);
  }
  // CHECK: KLEE: done: completed paths = 2
}
