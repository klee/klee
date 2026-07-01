// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc

// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-call-warnings=none %t.bc abc defg 2>&1 | FileCheck --check-prefix=CHECK-NONE %s

// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-call-warnings=once-per-function %t.bc abc defg 2>&1 | FileCheck --check-prefix=CHECK-ONCE %s

// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-call-warnings=all %t.bc abc defg 2>&1 | FileCheck --check-prefix=CHECK-ALL %s

#include "klee/klee.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  return strlen(argv[1]) + strlen(argv[2]);
  // CHECK-NONE-NOT: calling external

  // CHECK-ONCE: calling external
  // CHECK-ONCE-NOT: calling external

  // CHECK-ALL: calling external
  // CHECK-ALL: calling external
  // CHECK-ALL-NOT: calling external
}
