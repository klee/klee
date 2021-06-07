// ASAN fails because KLEE does not cleanup states with -dump-states-on-halt=false
// REQUIRES: not-asan
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -dump-states-on-halt=false -max-instructions=1 -output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

int main(int argc, char **argv) {
  // just do something
  int i = 42;
  ++i;
}

// CHECK: KLEE: done: completed paths = 0
// CHECK: KLEE: done: partially completed paths = 1