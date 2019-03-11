// RUN: %clang  -g -emit-llvm %O0opt -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --write-no-tests --exit-on-error %t.bc > %t.log
// RUN: FileCheck %s --input-file=%t.log

#include "klee/klee.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

unsigned klee_urange(unsigned start, unsigned end) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x, "x");
  if (x - start >= end - start)
    klee_silent_exit(0);
  return x;
}

int main(int argc, char **argv) {
  int a = klee_urange(0, 3);
  int b;

  // fork states
  switch (a) {
  case 0:
    b = -0;
    break;
  case 1:
    b = -1;
    break;
  case 2:
    b = -2;
    break;
  default:
    assert(0 && "Impossible switch target");
  }

  // test 80-bit external dispatch
  long double d = powl((long double)-11.0, (long double)a);
  // CHECK-DAG: powl(-11.0,0)=1.0
  // CHECK-DAG: powl(-11.0,1)=-11.0
  // CHECK-DAG: powl(-11.0,2)=121.0
  printf("powl(-11.0,%d)=%Lf\n", a, d);

  // test 80-bit fdiv
  long double e = (long double)1 / (long double)b;
  // CHECK-DAG: 1/0=inf
  // CHECK-DAG: 1/-1=-1.0
  // CHECK-DAG: 1/-2=-0.50
  printf("1/%d=%Lf\n", b, e);

  return 0;
}
