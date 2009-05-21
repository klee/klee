// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc

#include <assert.h>

int main() {
  char i, x[3];

  klee_make_symbolic(&i, sizeof i);

  x[0] = i;

  // DEMAND FAILED:Memory.cpp:read8:199: <isByteFlushed(offset)> is false: "unflushed byte without cache value"
  char y =  x[1];

  return 0;
}

