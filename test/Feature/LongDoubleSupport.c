// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --exit-on-error %t1.bc > %t2.out

#include <stdio.h>
#include <float.h>

// FIXME: This doesn't really work at all, it just doesn't
// crash. Until we have wide constant support, that is all we care
// about; the only reason this comes up is in initialization of
// constants, we don't actually end up seeing much code which uses long
// double.
int main() {
  long double a = LDBL_MAX;
  long double b = -1;
  long double c = a + b;
  printf("a = %Lg\n", a);
  printf("b = %Lg\n", b);
  printf("c = %Lg\n", c);
  return 0;
}
