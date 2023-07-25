// REQUIRES: z3
// RUN: %clang -DAllocSource1 %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t1.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t1.klee-out-1 --annotations=%S/AllocSource.json --mock-policy=all --mock-modeled-functions -emit-all-errors=true %t1.bc 2>&1 | FileCheck %s -check-prefix=CHECK-ALLOCSOURCE1

// RUN: %clang -DAllocSource2 %s -g -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t2.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t2.klee-out-1 --annotations=%S/AllocSource.json --mock-policy=all --mock-modeled-functions -emit-all-errors=true %t2.bc 2>&1 | FileCheck %s -check-prefix=CHECK-ALLOCSOURCE2

// RUN: %clang -DAllocSource3 %s -g -emit-llvm %O0opt -c -o %t3.bc
// RUN: rm -rf %t3.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t3.klee-out-1 --annotations=%S/AllocSource.json --mock-policy=all --mock-modeled-functions -emit-all-errors=true %t3.bc 2>&1 | FileCheck %s -check-prefix=CHECK-ALLOCSOURCE3

// RUN: %clang -DAllocSource4 %s -g -emit-llvm %O0opt -c -o %t4.bc
// RUN: rm -rf %t4.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t4.klee-out-1 --annotations=%S/AllocSource.json --mock-policy=all --mock-modeled-functions -emit-all-errors=true %t4.bc 2>&1 | FileCheck %s -check-prefix=CHECK-ALLOCSOURCE4

#include <assert.h>

#ifdef AllocSource1
void maybeAllocSource1(int *a);
#endif
void maybeAllocSource2(int **a);
int *maybeAllocSource3();
int **maybeAllocSource4();

int main() {
  int *a = 0;
#ifdef AllocSource1
  // CHECK-ALLOCSOURCE1: not valid annotation
  maybeAllocSource1(a);
  // CHECK-ALLOCSOURCE1: ASSERTION FAIL
  // CHECK-ALLOCSOURCE1: partially completed paths = 1
  // CHECK-ALLOCSOURCE1: generated tests = 1
#endif

#ifdef AllocSource2
  maybeAllocSource2(&a);
  // CHECK-NOT-ALLOCSOURCE2: memory error
  // CHECK-NOT-ALLOCSOURCE2: ASSERTION FAIL
  // CHECK-ALLOCSOURCE2: partially completed paths = 0
  // CHECK-ALLOCSOURCE2: generated tests = 1
#endif

#ifdef AllocSource3
  a = maybeAllocSource3();
  // CHECK-NOT-ALLOCSOURCE3 : memory error
  // CHECK-NOT-ALLOCSOURCE3: Not Allocated
  // CHECK-ALLOCSOURCE3: partially completed paths = 0
  // CHECK-ALLOCSOURCE3: generated tests = 1
#endif

#ifdef AllocSource4
  a = *maybeAllocSource4();
  // CHECK-NOT-ALLOCSOURCE4: ASSERTION FAIL
  // CHECK-ALLOCSOURCE4 : memory error
  // CHECK-ALLOCSOURCE4: partially completed paths = {{[1-9][0-9]*}}
  // CHECK-ALLOCSOURCE4: generated tests = {{[1-9][0-9]*}}
#endif

  assert(a != 0 && "Not Allocated");
  *a = 42;

  return 0;
}
