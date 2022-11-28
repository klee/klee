// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --only-output-states-covering-new --output-dir=%t.klee-out %t.bc > %t.log
// RUN: %klee-stats --print-columns 'ICov(%),BCov(%)' --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-STATS -input-file=%t.stats %s
#include "klee/klee.h"

#define a (2)
int main() {
  int i, n = 0, sn = 0;
  klee_make_symbolic(&n, sizeof(n), "n");
  for (i = 1; i <= n; i++) {
    sn = sn + a;
  }
  if (sn > 10) {
    printf("sn > 10");
  } else {
    printf("sn <= 10");
  }
}

// Check that there 100% coverage
// CHECK-STATS: ICov(%),BCov(%)
// CHECK-STATS: 100.00,100.00