// Darwin does not have strong aliases.
// REQUIRES: not-darwin
// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

#include <assert.h>

// Alias with a different value type (opaque pointers need no bitcast).
// NOTE: this does not have to be before b is known
extern short d __attribute__((alias("b")));

// alias for global
int b = 52;
extern int a __attribute__((alias("b")));

// alias for alias
// NOTE: this does not have to be before foo is known
extern int foo2() __attribute__((alias("foo")));

// alias for function
int __foo() { return 52; }
extern int foo() __attribute__((alias("__foo")));

// Alias with the same function type.
extern int foo3(void) __attribute__((alias("__foo")));

int *c = &a;

int main() {
  assert(a == 52);
  assert(*c == 52);
  assert((int)d == 52);

  assert(c == &b);
  // Volatile pointers keep Clang from folding alias address comparisons.
  short *volatile pd = &d;
  assert((int *)pd == &b);

  assert(foo() == 52);
  assert(foo2() == 52);
  assert(foo3() == 52);

  int (*volatile pfoo)(void) = foo;
  int (*volatile pfoo2)(void) = foo2;
  int (*volatile pfoo3)(void) = foo3;
  assert(pfoo == __foo);
  assert(pfoo2 == __foo);
  assert(pfoo3 == __foo);

  return 0;
}
