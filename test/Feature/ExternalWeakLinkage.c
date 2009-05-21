// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: %klee --exit-on-error %t1.bc

#include <assert.h>

void __attribute__((weak)) IAmSoWeak(int);

int main() {
  assert(IAmSoWeak==0);
  return 0;
}
