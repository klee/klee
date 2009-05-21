// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: %klee --libc=uclibc --exit-on-error %t1.bc

#include <assert.h>

int main() {
  printf("HOME: %s\n", getenv("HOME"));
  assert(getenv("HOME") != 0);
  return 0;
}
