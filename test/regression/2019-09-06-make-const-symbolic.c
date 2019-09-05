// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s
#include "klee/klee.h"

const int c = 23;
int main(int argc, char **argv) {
  klee_make_symbolic(&c, sizeof(c), "c");
  // CHECK: cannot make readonly object symbolic

  if (c > 20)
    return 0;
  else
    return 1;
}