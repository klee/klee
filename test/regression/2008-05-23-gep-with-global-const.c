// RUN: %llvmgcc -emit-llvm -O0 -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t.bc

#include <assert.h>

int a;

int main() {
  void *p1 = &((char*) 0)[(long) &a];
  void *p2 = &a;

  assert(p1 == p2);

  return 0;
}
