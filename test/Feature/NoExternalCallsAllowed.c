// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=none %t1.bc abc def 2>&1 | FileCheck %s
// RUN: test %t.klee-out/test000001.user.err
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  // CHECK: Disallowed call to external function: strlen
  int len = strlen(argv[0]);
  printf("%d\n", len);
}
