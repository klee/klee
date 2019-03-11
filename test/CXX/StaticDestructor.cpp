// don't optimize this, llvm likes to turn the *p into unreachable

// RUN: %clangxx %s -emit-llvm -g %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --optimize=false --libc=klee --write-no-tests %t1.bc 2> %t1.log
// RUN: FileCheck --input-file %t1.log %s

#include <cassert>

class Test {
  int *p;

public:
  Test() : p(0) {}
  ~Test() {
    assert(!p);
    // CHECK: :[[@LINE+1]]: memory error
    assert(*p == 10); // crash here
  }
};

Test t;

int main(int argc, char **argv) { return 0; }
