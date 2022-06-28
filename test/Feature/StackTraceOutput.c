// REQUIRES: geq-llvm-7.0
// RUN: %clang %s -emit-llvm %O0opt -g -c -fdiscard-value-names -o %t.bc
// RUN: rm -rf %t.klee-out-d
// RUN: %klee --output-dir=%t.klee-out-d %t.bc
// RUN: FileCheck -input-file=%t.klee-out-d/test000001.assert.err -check-prefix=CHECK-DISCARD %s

// RUN: %clang %s -emit-llvm %O0opt -g -c -fno-discard-value-names -o %t.bc
// RUN: rm -rf %t.klee-out-n
// RUN: %klee --output-dir=%t.klee-out-n %t.bc
// RUN: FileCheck -input-file=%t.klee-out-n/test000001.assert.err -check-prefix=CHECK-NODISCARD %s

#include "klee/klee.h"

#include <assert.h>

void foo(int i, int k) {
  ++i; ++k;
  assert(0);
  // CHECK-DISCARD: {{.*}} in foo(symbolic, 12) at {{.*}}.c:18
  // CHECK-DISCARD: {{.*}} in main() at {{.*}}.c:28
  // CHECK-NODISCARD: {{.*}} in foo(i=symbolic, k=12) at {{.*}}.c:18
  // CHECK-NODISCARD: {{.*}} in main() at {{.*}}.c:28
}

int main(void) {
  int i, k=12;
  klee_make_symbolic(&i, sizeof(i), "i");
  foo(i,k);
}
