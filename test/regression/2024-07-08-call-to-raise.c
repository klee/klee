// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --mock-policy=all --external-calls=all %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <signal.h>

int main() {
  int n;
  klee_make_symbolic(&n, sizeof(n), "n");

  if (n) {
    // CHECK: Call to "raise" is unmodelled
    raise(5);
  }
  // CHECK: KLEE: done: completed paths = 1
}
