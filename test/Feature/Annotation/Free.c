// REQUIRES: z3
// RUN: %clang -DFree1 %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t1.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t1.klee-out-1 --annotations=%S/Free.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t1.bc 2>&1 | FileCheck %s -check-prefix=CHECK-FREE1

// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t2.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t2.klee-out-1 --annotations=%S/Free.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t2.bc 2>&1 | FileCheck %s -check-prefix=CHECK-FREE2

// RUN: %clang -DFree3 %s -g -emit-llvm %O0opt -c -o %t3.bc
// RUN: rm -rf %t3.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t3.klee-out-1 --annotations=%S/Free.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t3.bc 2>&1 | FileCheck %s -check-prefix=CHECK-FREE3

#include <assert.h>

int *maybeAllocSource1();
void maybeFree1(int *a);

int main() {
  int *a;
#ifdef Free1
  // CHECK-FREE1: memory error: invalid pointer: free
  // CHECK-FREE1: KLEE: done: completed paths = 1
  // CHECK-FREE1: KLEE: done: partially completed paths = 1
  // CHECK-FREE1: KLEE: done: generated tests = 2
  a = malloc(sizeof(int));
  maybeFree1(a);
  maybeFree1(a);
#endif

  a = maybeAllocSource1();
  maybeFree1(a);
  // CHECK-NOT-FREE2: memory error: invalid pointer: free
  // CHECK-FREE2: KLEE: done: completed paths = 1
  // CHECK-FREE2: KLEE: done: partially completed paths = 0
  // CHECK-FREE2: KLEE: done: generated tests = 1
#ifdef Free3
  // CHECK-FREE3: memory error: invalid pointer: free
  // CHECK-FREE3: KLEE: done: completed paths = 0
  // CHECK-FREE3: KLEE: done: partially completed paths = 1
  // CHECK-FREE3: KLEE: done: generated tests = 1
  maybeFree1(a);
#endif
  return 0;
}
