// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %T/klee-last/test000001.ptr.err

int main() {
  int *x = malloc(sizeof(int));
  // CHECK: OneOutOfBounds.c:9: memory error: out of bound pointer
  // FIXME: Use FileCheck's relative line numbers
  x[1] = 1;
  free(x);
  return 0;
}
