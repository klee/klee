// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.free.err

int buf[4];

int main() {
  // CHECK: 2008-03-04-free-of-global.c:10: free of global
  free(buf); // this should give runtime error, not crash
  return 0;
}
