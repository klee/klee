// don't optimize this, llvm likes to turn the *p into unreachable

// RUN: %llvmgxx %s -emit-llvm -g -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --optimize=false --libc=klee --no-output %t1.bc 2> %t1.log
// RUN: grep ":17: memory error" %t1.log

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

int main(int argc, char** argv) {
  return 0;
}
