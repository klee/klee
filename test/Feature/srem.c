// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --klee-call-optimisation=false %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --klee-call-optimisation=false --optimize %t.bc 2>&1 | FileCheck %s

#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv)
{
    int y;

    klee_make_symbolic(&y, sizeof(y), "y");

    // Test cases divisor is positive or negative
    if (y >= 0) {
      if (y < 2) {
        // Two test cases generated taking this path, one for y == 0 and y == 1
        // CHECK: srem.c:[[@LINE+1]]: divide by zero
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

    // should fail for y == 0, succeed for all others
    // BUT: no new testcase is generated as y = 0 already provoked the first assert to fail
    assert(0 % y == 0);

    // should fail for y == 0 and y == +/-1, but succeed for all others
    // generates one testcase for either y == 1 or y == -1
    // CHECK: srem.c:[[@LINE+1]]: ASSERTION FAIL
    assert(-1 % y == -1);

    // CHECK: KLEE: done: completed paths = 5
}
