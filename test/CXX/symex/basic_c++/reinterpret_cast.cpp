// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc %t.bc 2>&1 | FileCheck %s

#include <cstdio>

int main(int argc, char **args) {
  unsigned int *x = new unsigned int(0);
  char *y = reinterpret_cast<char *>(x);

  printf("address(x) == %p\n", x);
  printf("address(y) == %p\n", y);
  // CHECK: address(x) == [[POINTER_X:0x[a-fA-F0-9]+]]
  // CHECK: address(y) == [[POINTER_X]]

  printf("first x == 0x%08X\n", *x);
  // CHECK: first x == 0x00000000
  y[3] = 0xAB;
  y[1] = 0x78;
  printf("second x == 0x%08X\n", *x);
  // CHECK: second x == 0xAB007800

  delete x;
  return 0;
}
