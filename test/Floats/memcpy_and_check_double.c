// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --output-dir=%t.klee-out --exit-on-error %t1.bc
#include "klee/klee.h"
#include <assert.h>
#include <string.h>

#define EXP_MASK 0x7ff0000000000000LL
#define SIGNIFICAND_MASK 0x000fffffffffffffLL

int main() {
  double f;
  klee_make_symbolic(&f, sizeof(double), "f");
  assert(sizeof(uint64_t) == sizeof(double));

  // Do some operation that means that f is more
  // than just a read of some bytes. It now is
  // an expression of type float.
  f = f + 1;

  uint64_t x = 0;
  memcpy(&x, &f, sizeof(double));

  // copy back
  double g;
  memcpy(&g, &x, sizeof(double));

  if ((x & EXP_MASK) == EXP_MASK) {
    // Number is either NaN of infinity
    if ((x & SIGNIFICAND_MASK) == 0) {
      // Number should be infinity
      assert(klee_is_infinite_double(f));
      assert(klee_is_infinite_double(g));
    } else {
      // Number should be NaN
      assert(klee_is_nan_double(f));
      assert(klee_is_nan_double(g));
    }
  }
  return 0;
}
