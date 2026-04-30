// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out %t.replay-out %t.replay
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors %t.bc
// RUN: mkdir -p %t.replay
// RUN: cp %t.klee-out/test*.ktest %t.replay/
// RUN: %klee --output-dir=%t.replay-out --replay-ktest-dir=%t.replay %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>

int main(void) {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  if (x == 0) {
    klee_assert(0 && "first replay error");
  }

  if (x == 1) {
    klee_assert(0 && "second replay error");
  }

  return 0;
}

// CHECK: KLEE: replaying:
// CHECK: KLEE: ERROR:
// CHECK: KLEE: replaying:
// CHECK: KLEE: ERROR:
// CHECK: KLEE: done:
