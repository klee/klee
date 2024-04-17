// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime --libc=klee   %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime --libc=uclibc %t.bc 2>&1 | FileCheck %s
#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void foo() { printf("called foo\n"); }

int main() {
  atexit(foo);

  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  // CHECK-NOT: called foo

  if (x == 0) {
    _exit(0);
  } else {
    _Exit(0);
  }

  // CHECK: KLEE: done: completed paths = 2
}
