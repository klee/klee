// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc
// RUN: ls %t.klee-out | not grep *.err

#include <stdlib.h>

int main() {
  free(0);
  return 0;
}
