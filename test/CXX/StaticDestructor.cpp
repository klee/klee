// XFAIL: darwin

// RUN: %clangxx %s -emit-llvm -g %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee %t1.bc 2> %t1.log
// RUN: FileCheck --input-file %t1.log %s

#include <cassert>

class Test {
  int x;

public:
  Test() : x(13) {}
  ~Test() {
    // CHECK: :[[@LINE+1]]: divide by zero
    x = x / (x - 13);
  }
};

Test t;

int main(int argc, char **argv) { return 0; }
