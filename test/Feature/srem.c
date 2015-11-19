// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -use-cex-cache=1 %t.bc
// RUN: grep "KLEE: done: explored paths = 5" %t.klee-out/info
// RUN: grep "KLEE: done: generated tests = 4" %t.klee-out/info
#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv)
{
    int y;

    klee_make_symbolic(&y, sizeof(y), "y");

    // Test cases divisor is positive or negative
    if (y >= 0) {
      if (y < 2) {
        // Two test cases generated taking this path, one for y == 0 and y ==1
        assert(1 % y == 0);
      } else {
        assert(1 % y == 1);
      }
    } else {
      if (y > -2) {
        assert(1 % y == 0);
      } else {
        assert(1 % y == 1);
      }
    }

    assert(0 % y == 0);
    assert(-1 % y == -1);
}
