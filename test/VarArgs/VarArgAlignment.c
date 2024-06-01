/* This test is for the alignment of variadic arguments.  In
   particular, on x86 arguments > 8 bytes (long double arguments in
   test1) should be 16-byte aligned, unless they are passed byval with 
   specified alignment */

// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --exit-on-error --output-dir=%t.klee-out %t1.bc | FileCheck %s

#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

void test1(int x, ...) {
  va_list ap;
  va_start(ap, x);
  int i1 = va_arg(ap, int);
  int i2 = va_arg(ap, int);
  int i3 = va_arg(ap, int);
  printf("i1, i2, i3: %d, %d, %d\n", i1, i2, i3);
  // CHECK: i1, i2, i3: 1, 2, 3

  long long int l1 = va_arg(ap, long long int);  
  printf("l1: %lld\n", l1);
  // CHECK: l1: 4

  int i4 = va_arg(ap, int);
  printf("i4: %d\n", i4);
  // CHECK: i4: 5
  
  long double ld1 = va_arg(ap, long double);
  printf("ld1: %Lf\n", ld1);
  // CHECK: ld1: 6.000000

  long double ld2 = va_arg(ap, long double);
  printf("ld2: %Lf\n", ld2);
  // CHECK: ld2: 7.000000

  
  va_end(ap);
}

struct mix {
  long long int first;
  char second;
};


void test2(int x, ...) {
  va_list ap;
  va_start(ap, x);
  int i1 = va_arg(ap, int);
  int i2 = va_arg(ap, int);
  int i3 = va_arg(ap, int);
  printf("i1, i2, i3: %d, %d, %d\n", i1, i2, i3);
  // CHECK: i1, i2, i3: 1, 2, 3

  long long int l1 = va_arg(ap, long long int);  
  printf("l1: %lld\n", l1);
  // CHECK: l1: 4

  int i4 = va_arg(ap, int);
  printf("i4: %d\n", i4);
  // CHECK: i4: 5
  
  struct mix m1 = va_arg(ap, struct mix);
  printf("m1: (%lld, %d)\n", m1.first, m1.second);
  // CHECK: m1: (7, 8)

  struct mix m2 = va_arg(ap, struct mix);
  printf("m2: (%lld, %d)\n", m2.first, m2.second);
  // CHECK: m2: (9, 10)
  
  va_end(ap);
}

struct baz {
  long double ll;
};

/* ld needs to be 16-byte aligned (checked with LLVM 9) */
void foo(int x, ...) {
  va_list ap;
  va_start(ap, x);
  int a = va_arg(ap, int);
  long double ld = va_arg(ap, long double);
  printf("ld: %Lf\n", ld);
  // CHECK: ld: 1.000000
}

/* b needs to be 8-byte aligned as specified via the align
   attribute (checked with LLVM 9) */
void bar(int x, ...) {
  va_list ap;
  va_start(ap, x);
  int a = va_arg(ap, int);
  struct baz b = va_arg(ap, struct baz);
  printf("b.ll: %Lf\n", b.ll);
  // CHECK: b.ll: 1.000000
}

int main() {
  int i1 = 1, i2 = 2, i3 = 3;
  long long int l1 = 4;
  int i4 = 5;
  long double ld1 = 6.0, ld2 = 7.0;
  struct mix m1 = { 7, 8 };
  struct mix m2 = { 9, 10 };
  test1(-1, i1, i2, i3, l1, i4, ld1, ld2);
  test2(-1, i1, i2, i3, l1, i4, m1, m2);

  long double ll = 1.0;
  struct baz s = { 1.0 };
  foo(1, 2, ll);
  bar(1, 3, s);

  return 0;
}
