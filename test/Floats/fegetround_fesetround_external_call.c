// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out -float-internals=0 --exit-on-error %t1.bc 0.5 > %t-output.txt 2>&1
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <assert.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  double rne;
  double ru;
  double rd;
  double rz;
} Result;

void setRoundingMode(int rm) {
  int result = fesetround(rm);
  assert(result == 0);
  int newRoundingMode = fegetround();
  assert(newRoundingMode != -1);
  assert(newRoundingMode == rm);
}

int main(int argc, char **argv) {
  assert(argc == 2);
  double initialValue = atof(
      argv[1]); // Get input value externally so it can't be constant folded.
  assert(initialValue == 0.5f);

  // Call an external function. Have to use double here otherwise fptrunc
  // instructions will get executed which respect the rounding mode but what
  // we're trying to test here is that the rounding mode is respected during
  // an external call.
  Result r;
  setRoundingMode(FE_UPWARD);
  // CHECK: calling external: sqrt
  r.rne = sqrt(initialValue);
  printf("Result RNE: %.40f (hexfloat: %a)\n", r.rne, r.rne);
  setRoundingMode(FE_TONEAREST);
  r.ru = sqrt(initialValue);
  printf("Result RU : %.40f (hexfloat: %a)\n", r.ru, r.ru);
  setRoundingMode(FE_DOWNWARD);
  r.rd = sqrt(initialValue);
  printf("Result RD : %.40f (hexfloat: %a)\n", r.rd, r.rd);
  setRoundingMode(FE_TOWARDZERO);
  r.rz = sqrt(initialValue);
  printf("Result RZ : %.40f (hexfloat: %a)\n", r.rz, r.rz);

  assert(r.rne == r.ru);
  assert(r.rd == r.rz);
  assert(r.rne > r.rd);
  return 0;
}
