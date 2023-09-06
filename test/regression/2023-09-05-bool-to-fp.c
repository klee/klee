// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__((__nothrow__, __leaf__)) __attribute__((__noreturn__));
void reach_error() { __assert_fail("0", "float3.c", 3, "reach_error"); }

double d = 0.0;

void f1() {
  d = 1.0;
}

int main() {
  int x = 2;

  if (klee_int("a"))
    x = 4;

  (void)f1();

  d += (x == 2);

  d += (x > 3);

  if (!(d == 2.0)) {
    reach_error();
    abort();
  }
}

// CHECK: KLEE: done: generated tests = 1
