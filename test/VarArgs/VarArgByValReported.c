/* This is the test reported in
   https://github.com/klee/klee/issues/189, checking the correctness
   of variadic arguments passed with the byval attribute */

// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --exit-on-error --output-dir=%t.klee-out %t1.bc | FileCheck %s

#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

struct triple {
  int first, second, third;
};

struct mix {
  long long int first;
  char second;
};

int test(int x, ...) {
  va_list ap;
  va_start(ap, x);
  int i1 = va_arg(ap, int);
  int i2 = va_arg(ap, int);
  int i3 = va_arg(ap, int);
  struct triple p = va_arg(ap, struct triple);
  struct mix m = va_arg(ap, struct mix);
  printf("types: (%d, %d, %d, (%d,%d,%d), (%lld,%d))\n",
          i1, i2, i3, p.first, p.second, p.third, m.first, m.second);
  // CHECK: types: (1, 2, 3, (9,12,15), (7,8))
  va_end(ap);
}

int main() {
  struct triple p = { 9, 12, 15 };
  struct mix m = { 7, 8 };
  test(-1, 1, 2, 3, p, m);

  return 0;
}
