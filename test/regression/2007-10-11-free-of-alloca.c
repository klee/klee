// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %T/klee-last/test000001.free.err

int main() {
  int buf[4];
  // CHECK: 2007-10-11-free-of-alloca.c:8: free of alloca
  free(buf); // this should give runtime error, not crash
  return 0;
}
