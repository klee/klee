// RUN: %llvmgxx %s --emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --no-output --exit-on-error --no-externals %t1.bc

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
