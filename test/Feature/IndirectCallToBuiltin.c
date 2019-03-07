// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc

#include <stdlib.h>
#include <stdio.h>

int main() {
  void *(*allocator)(size_t) = malloc;
  int *mem = allocator(10);

  printf("mem: %p\n", mem);
  printf("mem[0]: %d\n", mem[0]);

  return 0;
}
