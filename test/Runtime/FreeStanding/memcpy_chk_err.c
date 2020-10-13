// This test checks that __memcpy_chk find the kind of errors it was
// designed to find

// It requires clang >= 10, otherwise a direct call to memcpy is
// emitted instead of to __memcpy_chk
// REQUIRES: geq-llvm-10.0

// RUN: %clang %s -emit-llvm -O2 -g -c -D_FORTIFY_SOURCE=1 -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t2.bc

// RUN: test -f %t.klee-out/test000001.ptr.err
// RUN: FileCheck --input-file %t.klee-out/test000001.ptr.err %s
// CHECK: memcpy overflow

#include "klee/klee.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char d[5];
  char* s = "1234567890";

  memcpy(d, s, 10);
}
