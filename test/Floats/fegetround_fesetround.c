// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <assert.h>
#include <fenv.h>


int main() {
  int rm = fegetround();
  int result = 0;
  int newRm = 0;
  assert(rm == FE_TONEAREST);

  newRm = FE_UPWARD;
  result = fesetround(newRm);
  assert(result == 0 && "success");
  assert(fegetround() == newRm);

  newRm = FE_DOWNWARD;
  result = fesetround(newRm);
  assert(result == 0 && "success");
  assert(fegetround() == newRm);

  newRm = FE_TOWARDZERO;
  result = fesetround(newRm);
  assert(result == 0 && "success");
  assert(fegetround() == newRm);

  // Bad rounding mode
  int tryRm = 0x7ff3;
  result = fesetround(tryRm);
  assert(result != 0 && "expected failure");
  assert(fegetround() != tryRm);
  assert(fegetround() == newRm);
  return 0;
}
