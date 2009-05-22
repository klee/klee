// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc > %t1.log
// RUN: grep -c foo %t1.log | grep 5
// RUN: grep -c bar %t1.log | grep 3

#include <stdio.h>
#include <stdlib.h>

void foo() { printf("  foo()\n"); }
void bar() { printf("  bar()\n"); }

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

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
