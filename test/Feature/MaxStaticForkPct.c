// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -max-static-fork-pct=0.2 -max-static-pct-check-delay=1 --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck --check-prefix=CHECK-pt2 %s
// RUN: rm -rf %t.klee-out
// RUN: %klee -max-static-fork-pct=0.5 -max-static-pct-check-delay=1 --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck --check-prefix=CHECK-pt5 %s
// RUN: rm -rf %t.klee-out
// RUN: %klee -max-static-fork-pct=0.8 -max-static-pct-check-delay=1 --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck --check-prefix=CHECK-pt8 %s
// RUN: rm -rf %t.klee-out
// RUN: %klee -max-static-cpfork-pct=0.5 -max-static-pct-check-delay=1 --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck --check-prefix=CHECK-cppt5 %s

#include "klee/klee.h"
#include <stdio.h>

int main() {
#define N 4
  int x[10], i;
  klee_make_symbolic(&x, sizeof(x), "x");

  if (x[0])
      printf("x[0] ");
  else printf("NOT x[0] ");

  if (x[1])
  // CHECK-pt2: skipping fork and concretizing condition (MaxStatic*Pct limit reached)
  // CHECK-cppt5: skipping fork and concretizing condition (MaxStatic*Pct limit reached)
      printf("x[1] ");
  else printf("NOT x[1] ");

  for (i = 2; i < 10; i++) {
    if (x[i])
    // CHECK-pt5: skipping fork and concretizing condition (MaxStatic*Pct limit reached)
    // CHECK-pt8: skipping fork and concretizing condition (MaxStatic*Pct limit reached)
      printf("x[%d] ", i);
    else printf("NOT x[%d] ", i);
  }

  // CHECK-pt2: completed paths = 4
  // CHECK-pt5: completed paths = 8
  // CHECK-pt8: completed paths = 17

  // CHECK-cppt5: completed paths = 2
  return 0;
}
