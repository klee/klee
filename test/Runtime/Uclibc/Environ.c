// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --exit-on-error %t1.bc

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
  printf("HOME: %s\n", getenv("HOME"));
  assert(getenv("HOME") != 0);
  return 0;
}
