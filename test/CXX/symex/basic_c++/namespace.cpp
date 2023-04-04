// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc %t.bc 2>&1 | FileCheck %s

#include <cstdio>

namespace test {
int a() {
  return 2;
}
} // namespace test

int main(int argc, char **args) {
  printf("test::a() == %d\n", test::a());
  //CHECK: test::a() == 2
  return 0;
}
