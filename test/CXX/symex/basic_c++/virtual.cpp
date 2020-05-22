// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc %t.bc 2>&1

#include <cassert>

class Testclass {
public:
  virtual int vtl() {
    return 1;
  }
};

class Derivedclass : public Testclass {
};

class Overridingclass : public Derivedclass {
public:
  int vtl() override {
    return 2;
  }
};

int main(int argc, char **args) {
  Testclass x;
  Derivedclass y;
  Overridingclass z;

  assert(x.vtl() == 1);
  assert(y.vtl() == 1);
  assert(z.vtl() == 2);

  return 0;
}
