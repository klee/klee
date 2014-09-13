// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.free.err

int main() {
  int buf[4];
  // CHECK: 2007-10-11-free-of-alloca.c:9: free of alloca
  free(buf); // this should give runtime error, not crash
  return 0;
}
