// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --optimize=0 --exit-on-error %t1.bc > %t2.out

#include <stdio.h>
#include <float.h>
#include <assert.h>

// FIXME: This doesn't really work at all, it just doesn't
// crash. Until we have wide constant support, that is all we care
// about; the only reason this comes up is in initialization of
// constants, we don't actually end up seeing much code which uses long
// double.
int main() {
  unsigned N0 = 0, N1 = 0, N2 = 0;

  float V0 = .1;
  while (V0 != 0) {
    V0 *= V0;
    N0++;
  }
  double V1 = .1;
  while (V1 != 0) {
    V1 *= V1;
    N1++;
  }
  long double V2 = .1;
  while (V2 != 0) {
    V2 *= V2;
    N2++;
  }

  printf("counts: %d, %d, %d\n", N0, N1, N2);

  assert(N0 == 6);
  assert(N1 == 9);
  assert(N2 == 13);

  return 0;
}
