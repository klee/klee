// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -search=dfs --output-dir=%t.klee-out %t1.bc | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

#include <klee/klee.h>

void __attribute__ ((noinline)) fooXYZ() { printf("  foo()\n"); }
void __attribute__ ((noinline)) bar() { printf("  bar()\n"); }

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  // call once, so that it is not removed by optimizations
  // CHECK: bar()
  bar();

  // no aliases
  // CHECK: foo()
  fooXYZ();

  // First path: (x <= 10)
  // CHECK: foo()
  // CHECK: foo()
  // Second path: (x > 10 && x <= 20)
  // CHECK: bar()
  // CHECK: foo()
  // Third path: (x > 20)
  // CHECK: bar()
  // CHECK: bar()
  // CHECK: foo()

  if (x > 10)
  {
    // foo -> bar
    klee_alias_function_regex("foo.*", "bar");

    if (x > 20)
      fooXYZ();
  }

  fooXYZ();

  // undo
  klee_alias_undo("foo.*");
  fooXYZ();
}
