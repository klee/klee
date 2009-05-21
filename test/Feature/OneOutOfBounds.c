// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc
// RUN: test -f klee-last/test000001.ptr.err

int main() {
  int *x = malloc(4);
  x[1] = 1;
  free(x);
  return 0;
}
