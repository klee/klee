// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
// REQUIRES: x86_64
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

int main() {
  long double x, y, z;
  klee_make_symbolic(&x, sizeof(long double), "x");
  switch (__fpclassifyl(x)) {
  case FP_NAN:
    printf("nan\n");
    assert(isnan(x));
    assert(x != x);
    break;
  case FP_INFINITE: {
    printf("infinity\n");
    assert(!isfinite(x));
    int isInf = isinf(x);
    if (isInf == 1) {
      printf("+inf\n");
    } else if (isInf == -1) {
      printf("-inf\n");
    } else {
      klee_report_error(__FILE__, __LINE__, "Branch should not be reachable",
                        "fpfeas");
    }
    break;
  }
  case FP_ZERO:
    assert(x == 0.0);
    assert(isfinite(x));
    printf("zero\n");
    break;
  case FP_SUBNORMAL:
    assert(!isnormal(x));
    assert(isfinite(x));
    printf("subnormal\n");
    break;
  case FP_NORMAL:
    assert(isnormal(x));
    assert(isfinite(x));
    printf("normal\n");
    break;
  default:
    klee_report_error(__FILE__, __LINE__, "Branch should not be reachable",
                      "fpfeas");
    return 0;
  }
}
// CHECK-NOT: silently concretizing (reason: floating point)
// CHECK: KLEE: done: completed paths = 6
