// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <assert.h>

typedef enum KleeRoundingMode KleeRoundingModeTy;

int main() {
  KleeRoundingModeTy rm = klee_get_rounding_mode();
  assert(rm == KLEE_FP_RNE); // Correct default.

  klee_set_rounding_mode(KLEE_FP_RNA);
  assert(klee_get_rounding_mode() == KLEE_FP_RNA);

  klee_set_rounding_mode(KLEE_FP_RNA);
  assert(klee_get_rounding_mode() == KLEE_FP_RNA);

  klee_set_rounding_mode(KLEE_FP_RU);
  assert(klee_get_rounding_mode() == KLEE_FP_RU);

  klee_set_rounding_mode(KLEE_FP_RD);
  assert(klee_get_rounding_mode() == KLEE_FP_RD);

  klee_set_rounding_mode(KLEE_FP_RZ);
  assert(klee_get_rounding_mode() == KLEE_FP_RZ);
  return 0;
}
