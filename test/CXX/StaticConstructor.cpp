// RUN: %llvmgxx %s --emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --libc=klee --no-output --exit-on-error %t1.bc

#include <cassert>

// to make sure globals are initialized
int aGlobal = 21;

class Test {
  int x;

public:
  Test() : x(aGlobal + 1) {}
  ~Test() {}

  int getX() { return x; }
};

Test t;

int main() {
  assert(t.getX()==22);

  return 0;
}
