// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --only-output-states-covering-new --output-dir=%t.klee-out %t.bc > %t.log
// RUN: %klee-stats --print-columns 'ICov(%),BCov(%)' --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-STATS -input-file=%t.stats %s
#include "klee/klee.h"

int dec(int n) {
  return --n;
}

int fib(int n) {
  if (n == 0 || n == 1) {
    return 1;
  }
  return fib(dec(n)) + fib(dec(dec(n)));
}

int main() {
  int n;
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_assume(n >= 0);
  fib(n);
  return 0;
}

// Check that there 100% coverage
// CHECK-STATS: ICov(%),BCov(%)
// CHECK-STATS: 100.00,100.00