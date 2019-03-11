// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-no-tests --exit-on-error %t1.bc

#include <cassert>

class Test {
  int x;

public:
  Test(int _x) : x(_x) {}
  ~Test() {}

  int getX() { return x; }
};

int main() {
  Test rt(2);
  
  assert(rt.getX()==2);

  return 0;
}
