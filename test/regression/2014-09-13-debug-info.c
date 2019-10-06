// Check that we properly detect states covering new instructions.
//
// RUN: %clang -I../../../include %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --only-output-states-covering-new %t1.bc

// We expect 4 different output states, one for each named value and one "other"
// one with the prefered CEX. We verify this by using ktest-tool to dump the
// values, and then checking the output.
//
// RUN: /bin/sh -c "ktest-tool %t.klee-out/*.ktest" > %t.data-values
// RUN: FileCheck < %t.data-values %s

// CHECK-DAG: object 0: int : 0
// CHECK-DAG: object 0: int : 17
// CHECK-DAG: object 0: int : 32
// CHECK-DAG: object 0: int : 99

#include "klee/klee.h"

void f0(void) {}
void f1(void) {}
void f2(void) {}
void f3(void) {}

int main() {
  int x;
  klee_make_symbolic(&x, sizeof x, "x");
  klee_assume((unsigned) x < 256);

  if (x == 17) {
    f0();
  } else if (x == 32) {
    f1();
  } else if (x == 99) {
    f2();
  } else {
    klee_prefer_cex(&x, x == 0);
    f3();
  }

  return 0;
}
