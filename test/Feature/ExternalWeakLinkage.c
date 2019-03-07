// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

#include <assert.h>

void __attribute__((weak)) IAmSoWeak(int);

int main() {
  assert(IAmSoWeak==0);
  return 0;
}
