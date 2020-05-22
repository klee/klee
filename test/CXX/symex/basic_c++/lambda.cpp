// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc %t.bc 2>&1 | FileCheck %s

#include <cstdio>

int main(int argc, char **args) {
  int x = 0;
  auto y = [&](int h) { return x + h; };
  auto z = [=](int h) { return x + h; };
  x = 1;

  printf("y(0) == %d\n", y(0));
  // CHECK: y(0) == 1
  printf("z(0) == %d\n", z(0));
  // CHECK: z(0) == 0

  return 0;
}
