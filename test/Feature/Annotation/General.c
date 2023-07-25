// REQUIRES: z3
// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t1.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t1.klee-out-1 --annotations=%S/BrokenAnnotation.json --mock-policy=all -emit-all-errors=true %t1.bc 2>%t.stderr.log || echo "Exit status must be 0"
// RUN: FileCheck -check-prefix=CHECK-JSON --input-file=%t.stderr.log %s
// CHECK-JSON: Annotation: Parsing JSON

// RUN: %clang -DPTRARG %s -g -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t2.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t2.klee-out-1 --annotations=%S/General.json --mock-policy=all -emit-all-errors=true %t2.bc 2>&1 | FileCheck %s -check-prefix=CHECK

// RUN: %clang -DPTRRET %s -g -emit-llvm %O0opt -c -o %t3.bc
// RUN: rm -rf %t3.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t3.klee-out-1 --annotations=%S/General.json --mock-policy=all -emit-all-errors=true %t3.bc 2>&1 | FileCheck %s -check-prefix=CHECK

#include <assert.h>

struct ST {
  int a;
  int b;
};

void ptrArg(struct ST, int **a);
int *ptrRet();

int main() {
  int *a = 15;
#ifdef PTRARG
  struct ST st;
  ptrArg(st, &a);
#endif

#ifdef PTRRET
  a = ptrRet();
#endif

  // CHECK: memory error: null pointer exception
  // CHECK: partially completed paths = 1
  // CHECK: generated tests = 2
  *a = 42;

  return 0;
}
