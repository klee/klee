// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out  %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
// RUN: FileCheck -input-file=%t-output.txt -check-prefix=MSG %s
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <assert.h>
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum KleeRoundingMode KleeRoundingModeTy;

int main() {
  int symbolicRoundingMode = FE_TONEAREST;
  klee_make_symbolic(&symbolicRoundingMode, sizeof(int), "rounding_mode");

  // Set a symbolic rounding mode.
  // This should result in five paths leaving the call.
  int result = fesetround(symbolicRoundingMode);

  int newRoundingMode = fegetround();
  assert(!klee_is_symbolic(newRoundingMode));
  assert(newRoundingMode != -1);

  // This assert will fail if the fesetround() call failed.
  // CHECK-DAG: fegetround_fesetround_symbolic.c:[[@LINE+1]]: ASSERTION FAIL: newRoundingMode == symbolicRoundingMode
  assert(newRoundingMode == symbolicRoundingMode);

  switch (newRoundingMode) {
#define SUCCESSFUL_CASE(X) \
  case X:                  \
    printf(#X "\n");       \
    assert(result == 0);   \
    break;
    SUCCESSFUL_CASE(FE_TONEAREST)
    SUCCESSFUL_CASE(FE_UPWARD)
    SUCCESSFUL_CASE(FE_DOWNWARD)
    SUCCESSFUL_CASE(FE_TOWARDZERO)
  default:
    // MSG-NOT: unreachable
    printf("unreachable\n");
    abort();
  }
  return 0;
}

// CHECK-DAG: FE_TONEAREST
// CHECK-DAG: FE_UPWARD
// CHECK-DAG: FE_DOWNWARD
// CHECK-DAG: FE_TOWARDZERO

// CHECK-DAG: KLEE: done: completed paths = 4
