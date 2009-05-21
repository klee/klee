// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --exit-on-error %t1.bc

#include <stdlib.h>

int main() {
  // concrete case
  void *p = malloc(0);
  free(p);

  p = malloc(0);
  void *arr[4] = { p, 0, 0, 0 };

  // symbolic case
  free(arr[klee_range(0, 4, "range")]);
}
