// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc
// RUN: ls %T/klee-last | not grep *.err

#include <stdlib.h>

int main() {
  free(0);
  return 0;
}
