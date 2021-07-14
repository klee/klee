// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --output-dir=%t.klee-out --exit-on-error %t1.bc
#include "klee/klee.h"
#include <assert.h>
#include <string.h>

#define EXP_MASK 0x7f800000
#define SIGNIFICAND_MASK 0x007fffff

int main() {
  float f;
  klee_make_symbolic(&f, sizeof(float), "f");
  assert(sizeof(int) == sizeof(float));

  // Do some operation that means that f is more
  // than just a read of some bytes. It now is
  // an expression of type float.
  f = f + 1;

  int x = 0;
  memcpy(&x, &f, sizeof(float));

  // copy back
  float g;
  memcpy(&g, &x, sizeof(float));

  if ( (x & EXP_MASK) == EXP_MASK) {
    // Number is either NaN of infinity
    if ( (x & SIGNIFICAND_MASK) == 0) {
      // Number should be infinity
      assert(klee_is_infinite_float(f));
      assert(klee_is_infinite_float(g));
    } else {
      // Number should be NaN
      assert(klee_is_nan_float(f));
      assert(klee_is_nan_float(g));
    }
  }
  return 0;
}
