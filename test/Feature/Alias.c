// Darwin does not have strong aliases.
// REQUIRES: not-darwin
// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

#include <assert.h>

// alias for global
int b = 52;
extern int a __attribute__((alias("b")));

// alias for alias
// NOTE: this does not have to be before foo is known
extern int foo2() __attribute__((alias("foo")));

// alias for function
int __foo() { return 52; }
extern int foo() __attribute__((alias("__foo")));

int *c = &a;

int main() {
  assert(a == 52);
  assert(foo() == 52);
  assert(foo2() == 52);
  assert(*c == 52);

  return 0;
}
