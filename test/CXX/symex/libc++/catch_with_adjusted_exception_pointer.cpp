// REQUIRES: uclibc
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -std=c++11 -c %libcxx_includes -g -nostdinc++ -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libcxx --libc=uclibc %t.bc 2>&1 | FileCheck %s

#include <iostream>

class Base {
public:
  int base_int = 42;
  virtual void print() {
    std::cout << "Base\n";
  }
};

class Derived : public Base {
public:
  virtual void print() {
    std::cout << "Derived\n";
  }
};

int main() {
  Base *bd = new Derived();
  // CHECK: Derived
  bd->print();

  try {
    throw bd;
  } catch (Base *b) {
    // CHECK: Derived
    b->print();
    // CHECK: 42
    std::cout << b->base_int << "\n";
  }
}
