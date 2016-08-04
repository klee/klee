// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -exit-on-error-type Assert %t1.bc 2>&1

#include <assert.h>
#include <klee/klee.h>

int main() {
  assert(klee_int("assert"));

  while (1) {
  }

  return 0;
}
