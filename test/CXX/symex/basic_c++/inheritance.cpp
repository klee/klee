// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc %t.bc 2>&1 | FileCheck %s

#include <cstdio>
class Testclass {
public:
  Testclass() {
    printf("Testclass constructor\n");
  }
  int a() {
    return 1;
  }
};
class Derivedclass : public Testclass {
public:
  Derivedclass() {
    printf("Derivedclass constructor\n");
  }
  int b() {
    return 2;
  }
};

int main(int argc, char **args) {
  Derivedclass x;
  // CHECK: Testclass constructor
  // CHECK: Derivedclass constructor

  printf("%d / %d", x.a(), x.b());
  // CHECK: 1 / 2

  return x.a() + x.b();
}
