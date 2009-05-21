// RUN: %llvmgxx %s --emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --no-output --exit-on-error %t1.bc

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
