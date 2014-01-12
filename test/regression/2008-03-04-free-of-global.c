// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %T/klee-last/test000001.free.err

int buf[4];

int main() {
  // CHECK: 2008-03-04-free-of-global.c:9: free of global
  free(buf); // this should give runtime error, not crash
  return 0;
}
