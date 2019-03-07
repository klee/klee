// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.ptr.err

int main() {
  int *x = malloc(4);
  free(x);
  // CHECK: memory error: invalid pointer: free
  free(x);
  return 0;
}
