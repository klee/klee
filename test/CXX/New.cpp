// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-no-tests --exit-on-error --external-calls=none %t1.bc

#include <cassert>

class Test {
  int x;

public:
  Test(int _x) : x(_x) {
  }
  ~Test() {
  }

  int getX() { return x; }
};

// This doesn't do what I want because
// it is being changed to alloca, but
// it is also failing.
int main(int argc) {
  Test *rt = new Test(2);
  
  assert(rt->getX()==2);

  delete rt;

  return 0;
}
