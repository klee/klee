// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %T/klee-last/test000001.ptr.err

int main() {
  int *x = malloc(4);
  free(x);
  // CHECK: memory error: invalid pointer: free
  free(x);
  return 0;
}
