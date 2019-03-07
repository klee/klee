// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=none %t1.bc 2>&1 | FileCheck %s
// RUN: test %t.klee-out/test000001.user.err

#include <stdio.h>

int main(int argc, char** argv) {
  // CHECK: Disallowed call to external function: abs
  int x = abs(argc);
  printf("%d\n", argc);
  return 0;
}
