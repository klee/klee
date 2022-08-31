// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

int main() {
  int *ptr;
  klee_make_symbolic(&ptr, sizeof(ptr), "ptr");
  // CHECK: NullPointerDereferenceCheck.c:[[@LINE+1]]: memory error: null pointer exception
  *ptr = 10;
}
