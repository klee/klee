// REQUIRES: not-asan
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s
// CHECK: failed external call: strdup
// CHECK: failed external call: strdup

// objective: check handling of more than one failing external call


#include "klee/klee.h"

#include <stdbool.h>
#include <string.h>

int main(int argc, char * argv[]) {
  bool b;
  klee_make_symbolic(&b, sizeof(bool), "b");

  char * s0;
  if (b) {
    s0 = strdup((char *) 0xdeadbeef);
  } else {
    s0 = strdup((void *) 0xdeafbee5);
  }
  (void) s0;
}
