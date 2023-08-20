// REQUIRES: z3
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t1.klee-out
// RUN: rm -rf %t2.klee-out
// RUN: %klee --output-dir=%t1.klee-out --use-guided-search=none --cex-cache-validity-cores=false --solver-backend=z3 %t1.bc
// RUN: %klee --output-dir=%t2.klee-out --use-guided-search=none --cex-cache-validity-cores=true --solver-backend=z3 %t1.bc
// RUN: %klee-stats --print-columns 'QCexCacheHits,SolverQueries' --table-format=csv %t1.klee-out > %t1.stats
// RUN: %klee-stats --print-columns 'QCexCacheHits,SolverQueries' --table-format=csv %t2.klee-out > %t2.stats
// RUN: FileCheck -check-prefix=CHECK-CACHE-OFF -input-file=%t1.stats %s
// RUN: FileCheck -check-prefix=CHECK-CACHE-ON -input-file=%t2.stats %s
#include "klee/klee.h"

int main(int argc, char **argv) {
  int a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (b == a + i && b == a + j)
        break;
    }
  }

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (b > i && b > j && b == a + i && b == a + j)
        break;
    }
  }
}
// CHECK-CACHE-ON: QCexCacheHits,SolverQueries
// CHECK-CACHE-ON: 1460,202
// CHECK-CACHE-OFF: QCexCacheHits,SolverQueries
// CHECK-CACHE-OFF: 1010,652
