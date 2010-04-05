// don't optimize this, llvm likes to turn the *p into unreachable

// RUN: %llvmgxx %s -emit-llvm -g -O0 -c -o %t1.bc
// RUN: %klee --libc=klee --no-output %t1.bc 2> %t1.log
// RUN: grep ":16: memory error" %t1.log

#include <cassert>

class Test {
  int *p;

public:
  Test() : p(0) {}
  ~Test() { 
    assert(!p); 
    assert(*p == 10); // crash here
  }
};

Test t;

int main() {
  return 0;
}
