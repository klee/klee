// Variable GEP indices must be recomputed, not accumulated, when
// bindModuleConstants() runs for each runFunctionAsMain() call. Without the
// clear, the second replay in this test has two copies of every variable
// index, and a[i] silently reads a[2*i].

// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out %t.replay-out %t.replay
// RUN: %klee --output-dir=%t.klee-out %t.bc
// RUN: mkdir -p %t.replay
// RUN: cp %t.klee-out/test000001.ktest %t.replay/test1.ktest
// RUN: cp %t.klee-out/test000001.ktest %t.replay/test2.ktest
// RUN: %klee --output-dir=%t.replay-out --replay-ktest-dir=%t.replay %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main(void) {
  int i;
  klee_make_symbolic(&i, sizeof i, "i");
  klee_assume(i == 1); // single path: exactly one ktest is generated

  volatile int a[4] = {10, 11, 12, 13};
  int v = a[i]; // must read a[1] == 11 in every replay

  if (v != 11)
    klee_report_error(__FILE__, __LINE__, "variable GEP index applied twice",
                      "gep.err");
  return 0;
}

// Replaying the same input twice must take the same path both times.
// CHECK-NOT: variable GEP index applied twice
// CHECK: KLEE: done: completed paths = 2
