// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=klee %t1.bc

// make sure the functions passed to atexit are called in reverse order

#include <assert.h>

extern void atexit(void (*)());

static int counter = 0;

void first() {
  assert(counter == 0);
  ++counter;
}

void second() {
  assert(counter == 1);
  ++counter;
}

int main() {
  atexit(second);
  atexit(first);
  return 0;
}
