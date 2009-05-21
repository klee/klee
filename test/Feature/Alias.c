// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: %klee --exit-on-error %t1.bc

// Darwin does not have strong aliases.
// XFAIL: darwin

#include <assert.h>

// alias for global
int b = 52;
extern int a __attribute__((alias("b")));

// alias for function
int __foo() { return 52; }
extern int foo() __attribute__((alias("__foo")));

int *c = &a;

int main() { 
  assert(a == 52);
  assert(foo() == 52);
  assert(*c == 52);
  
  return 0;
}
