// RUN: %llvmgcc -c -o %t1.bc %s
// RUN: %klee --exit-on-error %t1.bc

#include <stdio.h>
#include <assert.h>

int main() {
  int x = klee_int("x");
  klee_assume(x > 10);
  klee_assume(x < 20);

  assert(!klee_is_symbolic(klee_get_value_i32(x)));
  assert(klee_get_value_i32(x) > 10);
  assert(klee_get_value_i32(x) < 20);

  return 0;
}
