// Check for calling external functions using symbolic parameters.
// Externals calls might modify memory objects that have been previously fully symbolic.
// The constant modifications should be propagated back.
// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -external-calls=all %t.bc > %t.log
// RUN: FileCheck -input-file=%t.log %s
// REQUIRES: not-darwin
#include "klee/klee.h"
#include <stdio.h>

int main() {

  int a;
  int b;
  int c;

  // Symbolize argument that gets concretized by the external call
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");
  klee_make_symbolic(&c, sizeof(c), "c");
  if (a == 2 && b == 3) {
    // Constrain fully symbolic `a` and `b` to concrete values

    // Although a and b are not character vectors, explicitly constraining them to `2` and `3`
    // leads to the most significant bits of the int (e.g. bit 8 to 31 of 32bit) set to `\0`.
    // This leads to an actual null-termination of the character vector, which makes it safe
    // to use by an external function.
    printf("%s%s%n\n", (char *)&a, (char *)&b, &c);
    printf("after: a = %d b = %d c = %d\n", a, b, c);
    //CHECK: after: a = 2 b = 3 c = 2
  }

  return 0;
}
