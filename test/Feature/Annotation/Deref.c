// REQUIRES: z3
// RUN: %clang -DDeref1 %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t1.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t1.klee-out-1 --annotations=%S/Deref.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t1.bc 2>&1 | FileCheck %s -check-prefix=CHECK-DEREF1

// RUN: %clang -DDeref2 %s -g -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t2.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t2.klee-out-1 --annotations=%S/Deref.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t2.bc 2>&1 | FileCheck %s -check-prefix=CHECK-DEREF2

// RUN: %clang -DDeref3 %s -g -emit-llvm %O0opt -c -o %t3.bc
// RUN: rm -rf %t3.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t3.klee-out-1 --annotations=%S/Deref.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t3.bc 2>&1 | FileCheck %s -check-prefix=CHECK-DEREF3

// RUN: %clang -DDeref4 %s -g -emit-llvm %O0opt -c -o %t4.bc
// RUN: rm -rf %t4.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t4.klee-out-1 --annotations=%S/Deref.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t4.bc 2>&1 | FileCheck %s -check-prefix=CHECK-DEREF4

// RUN: %clang -DDeref5 %s -g -emit-llvm %O0opt -c -o %t5.bc
// RUN: rm -rf %t5.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t5.klee-out-1 --annotations=%S/Deref.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t5.bc 2>&1 | FileCheck %s -check-prefix=CHECK-DEREF5

// RUN: %clang -DDeref6 %s -g -emit-llvm %O0opt -c -o %t6.bc
// RUN: rm -rf %t6.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t6.klee-out-1 --annotations=%S/Deref.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t6.bc 2>&1 | FileCheck %s -check-prefix=CHECK-DEREF6

// RUN: %clang -DHASVALUE -DDeref1 -DDeref2 -DDeref3 -DDeref4 -DDeref5 -DDeref6 %s -g -emit-llvm %O0opt -c -o %t7.bc
// RUN: rm -rf %t7.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t7.klee-out-1 --annotations=%S/Deref.json --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t7.bc 2>&1 | FileCheck %s -check-prefix=CHECK-NODEREF
// CHECK-NODEREF-NOT: memory error: null pointer exception

// RUN: %clang -DDeref1 -DDeref2 -DDeref3 -DDeref4 -DDeref5 -DDeref6 %s -g -emit-llvm %O0opt -c -o %t8.bc
// RUN: rm -rf %t8.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t8.klee-out-1 --annotations=%S/EmptyAnnotation.json --external-calls=all --mock-policy=all --mock-modeled-functions --emit-all-errors=true %t8.bc 2>&1 | FileCheck %s -check-prefix=CHECK-EMPTY
// CHECK-EMPTY-NOT: memory error: null pointer exception
// CHECK-EMPTY: partially completed paths = 0
// CHECK-EMPTY: generated tests = 1

void maybeDeref1(int *a);
void maybeDeref2(int **a);
void maybeDeref3(int **a);
void maybeDeref4(int b, int *a);
void maybeDeref5(int *a, int b);
void maybeDeref6(int *a, int *b);

int main() {
  int c = 42;
#ifdef HASVALUE
  int *a = &c;
  int **b = &a;
#else
  int *a = 0;
  int **b = 0;
#endif

#ifdef Deref1
  // CHECK-DEREF1: memory error: null pointer exception
  maybeDeref1(a);
  // CHECK-DEREF1: memory error: out of bound pointer
  maybeDeref1((int *)42);
  // CHECK-DEREF1: partially completed paths = 2
  // CHECK-DEREF1: generated tests = 3
#endif

#ifdef Deref2
  // CHECK-DEREF2: memory error: null pointer exception
  maybeDeref2(b);
  maybeDeref2(&a);
  // CHECK-DEREF2: partially completed paths = 1
  // CHECK-DEREF2: generated tests = 3
#endif

#ifdef Deref3
  // CHECK-DEREF3: memory error: null pointer exception
  maybeDeref3(&a);
  // CHECK-DEREF3: memory error: null pointer exception
  maybeDeref3(b);
  // CHECK-DEREF3: partially completed paths = 2
  // CHECK-DEREF3: generated tests = 2
#endif

#ifdef Deref4
  // CHECK-DEREF4: memory error: null pointer exception
  maybeDeref4(0, a);
  // CHECK-DEREF4: partially completed paths = 1
  // CHECK-DEREF4: generated tests = 2
#endif

#ifdef Deref5
  // CHECK-DEREF5: memory error: null pointer exception
  maybeDeref5(a, 0);
  // CHECK-DEREF5: partially completed paths = 1
  // CHECK-DEREF5: generated tests = 2
#endif

#ifdef Deref6
  // CHECK-DEREF6: memory error: null pointer exception
  maybeDeref6(a, &c);
  // CHECK-DEREF6: memory error: null pointer exception
  maybeDeref6(&c, a);
  // CHECK-DEREF6: partially completed paths = 2
  // CHECK-DEREF6: generated tests = 3
#endif
  return 0;
}
