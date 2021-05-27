// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
// REQUIRES: x86_64
#include "klee/klee.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

int main() {
  long double x;
  klee_make_symbolic(&x, sizeof(long double), "x");
  if (klee_is_subnormal_long_double(x)) {
    assert(__fpclassifyl(x) == FP_SUBNORMAL);
  } else {
    assert(!klee_is_subnormal_long_double(x));
    // now does not fork in ``fpclassify()`` if it is compiled into a library call
    assert(__fpclassifyl(x) != FP_SUBNORMAL);
  }
}
// CHECK-NOT: silently concretizing (reason: floating point)
// CHECK: KLEE: done: completed paths = 2
