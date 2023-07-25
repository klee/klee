// REQUIRES: z3
// RUN: %clang -DInitNull1 %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t1.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t1.klee-out-1 --annotations=%S/InitNull.json --mock-policy=all -emit-all-errors=true %t1.bc 2>&1 | FileCheck %s -check-prefix=CHECK-INITNULL1

// RUN: %clang -DInitNull2 %s -g -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t2.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t2.klee-out-1 --annotations=%S/InitNull.json --mock-policy=all -emit-all-errors=true %t2.bc 2>&1 | FileCheck %s -check-prefix=CHECK-INITNULL2

// RUN: %clang -DInitNull3 %s -g -emit-llvm %O0opt -c -o %t3.bc
// RUN: rm -rf %t3.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t3.klee-out-1 --annotations=%S/InitNull.json --mock-policy=all -emit-all-errors=true %t3.bc 2>&1 | FileCheck %s -check-prefix=CHECK-INITNULL3

// RUN: %clang -DInitNull4 %s -g -emit-llvm %O0opt -c -o %t4.bc
// RUN: rm -rf %t4.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t4.klee-out-1 --annotations=%S/InitNull.json --mock-policy=all -emit-all-errors=true %t4.bc 2>&1 | FileCheck %s -check-prefix=CHECK-INITNULL4

// RUN: %clang -DInitNull1 -DInitNull2 -DInitNull3 -DInitNull4 %s -g -emit-llvm %O0opt -c -o %t5.bc
// RUN: rm -rf %t5.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t5.klee-out-1 --annotations=%S/InitNullEmpty.json --mock-policy=all -emit-all-errors=true %t5.bc 2>&1 | FileCheck %s -check-prefix=CHECK-EMPTY
// CHECK-EMPTY: ASSERTION FAIL
// CHECK-EMPTY: partially completed paths = {{[1-2]}}

// RUN: %clang -DMustInitNull5 %s -g -emit-llvm %O0opt -c -o %t6.bc
// RUN: rm -rf %t6.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t6.klee-out-1 --annotations=%S/InitNull.json --mock-policy=all -emit-all-errors=true %t6.bc 2>&1 | FileCheck %s -check-prefix=CHECK-INITNULL5

#include <assert.h>

#ifdef InitNull1
void mustInitNull1(int *a);
#endif
void mustInitNull2(int **a);
int *maybeInitNull1();
int **maybeInitNull2();

int *mustInitNull3();

int main() {
  int c = 42;
  int *a = &c;
#ifdef InitNull1
  // CHECK-INITNULL1: not valid annotation
  mustInitNull1(a);
  // CHECK-INITNULL1-NOT: A is Null
  // CHECK-INITNULL1: partially completed paths = 0
  // CHECK-INITNULL1: generated tests = 1
#endif

#ifdef InitNull2
  mustInitNull2(&a);
  // CHECK-INITNULL2: ASSERTION FAIL
  // CHECK-INITNULL2: partially completed paths = 1
  // CHECK-INITNULL2: generated tests = 2
#endif

#ifdef InitNull3
  a = maybeInitNull1();
  // CHECK-INITNULL3: ASSERTION FAIL
  // CHECK-INITNULL3: partially completed paths = 1
  // CHECK-INITNULL3: generated tests = 2
#endif

#ifdef InitNull4
  a = *maybeInitNull2();
  // CHECK-INITNULL4: ASSERTION FAIL
  // CHECK-INITNULL4: partially completed paths = {{[2-3]}}
#endif

#ifdef MustInitNull5
  a = mustInitNull3();
  // CHECK-INITNULL5: partially completed paths = 0
  // CHECK-INITNULL5: generated tests = 2
#else
  assert(a != 0 && "A is Null");
#endif

  return 0;
}
