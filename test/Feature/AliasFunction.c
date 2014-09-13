// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc > %t1.log
// RUN: grep -c foo %t1.log | grep 5
// RUN: grep -c bar %t1.log | grep 4

#include <stdio.h>
#include <stdlib.h>

void __attribute__ ((noinline)) foo() { printf("  foo()\n"); }
void __attribute__ ((noinline)) bar() { printf("  bar()\n"); }

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  // call once, so that it is not removed by optimizations
  bar();

  // no aliases
  foo();

  if (x > 10)
  {
    // foo -> bar
    klee_alias_function("foo", "bar");

    if (x > 20)
      foo();
  }
  
  foo();

  // undo
  klee_alias_function("foo", "foo");
  foo();
}
