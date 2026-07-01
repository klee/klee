// This test checks that symbolic arguments to a function call are correctly concretized
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc

// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=all --exit-on-error %t.bc abc def 2>&1 | FileCheck %s

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");
  // x is 0 or 1; argv[1] is "abc"; argv[2] is "def"
  klee_assume(x >= 1 & x <= 2);

  int len = strlen(argv[x]);
  printf("len = %d\n", len);
  // CHECK: calling external: strlen

  assert(len == 3);
}
