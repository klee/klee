// REQUIRES: uclibc
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -std=c++11 -c %libcxx_includes -g -nostdinc++ -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libcxx --libc=uclibc %t.bc 2>&1 | FileCheck %s

#include <cstdio>

class TestClass {
public:
  virtual ~TestClass() {}
};

class DerivedClass : public TestClass {
};

class SecondDerivedClass : public DerivedClass {
};

int main(int argc, char **args) {
  try {
    try {
      throw SecondDerivedClass();
    } catch (DerivedClass &p) {
      puts("DerivedClass caught\n");
      // CHECK: DerivedClass caught
      throw;
    }
  } catch (TestClass &tc) {
    puts("TestClass caught\n");
    // CHECK: TestClass caught
  }

  return 0;
}
