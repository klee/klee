// REQUIRES: geq-llvm-15.0
/* This test checks that KLEE correctly handles variadic arguments with the
   byval attribute */

// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --exit-on-error --output-dir=%t.klee-out %t1.bc
// RUN: FileCheck %s --input-file=%t.klee-out/assembly.ll
//
// TODO: Make noundef unconditional when LLVM 14 is the oldest supported version.
// CHECK: call void (ptr, i32, ...) @test1(ptr sret(%struct.foo) align 8 {{.*}}, i32 noundef -1, ptr noundef byval(%struct.foo) align 8 {{.*}}, ptr noundef byval(%struct.bar) align 8 {{.*}})
// CHECK: call void (ptr, i32, i64, ...) @test2(ptr sret(%struct.foo) align 8 {{.*}}, i32 noundef {{.*}}, i64 noundef {{.*}}, i32 noundef {{.*}}, ptr noundef byval(%struct.foo) align 8 {{.*}}, i64 noundef {{.*}}, ptr noundef byval(%struct.bar) align 8 {{.*}}, ptr noundef byval(%struct.foo) align 8 {{.*}}, ptr noundef byval(%struct.bar) align 8 {{.*}})
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

struct foo {
  char f1;
  long long f2;
  long long f3;
  char f4;
  long f5;
  int f6;
  char f7;
};

struct bar {
  long long int f1;
  char f2;
  long long f3;
  char f4;
  long f5;
};

struct foo test1(int x, ...) {
  va_list ap;
  va_start(ap, x);
  assert(x == -1);

  struct foo f = va_arg(ap, struct foo);
  assert(f.f1 == 1);
  assert(f.f2 == 2);
  assert(f.f3 == 3);
  assert(f.f4 == 4);
  assert(f.f5 == 5);
  assert(f.f6 == 6);
  assert(f.f7 == 7);
  
  struct bar b = va_arg(ap, struct bar);
  assert(b.f1 == 11);
  assert(b.f2 == 12);
  assert(b.f3 == 13);
  assert(b.f4 == 14);
  assert(b.f5 == 15);
  
  va_end(ap);

  f.f1++;
  return f;
}

struct foo test2(int x, long long int l, ...) {
  va_list ap;
  va_start(ap, l);
  assert(x == 10);
  assert(l == 1000);

  int i = va_arg(ap, int);
  assert(i == 10);

  struct foo f = va_arg(ap, struct foo);
  assert(f.f1 == 1);
  assert(f.f2 == 2);
  assert(f.f3 == 3);
  assert(f.f4 == 4);
  assert(f.f5 == 5);
  assert(f.f6 == 6);
  assert(f.f7 == 7);

  l = va_arg(ap, long long int);
  assert(l == 1000);
  
  struct bar b = va_arg(ap, struct bar);
  assert(b.f1 == 11);
  assert(b.f2 == 12);
  assert(b.f3 == 13);
  assert(b.f4 == 14);
  assert(b.f5 == 15);

  f = va_arg(ap, struct foo);
  assert(f.f1 == 10);
  assert(f.f2 == 20);
  assert(f.f3 == 30);
  assert(f.f4 == 40);
  assert(f.f5 == 50);
  assert(f.f6 == 60);
  assert(f.f7 == 70);

  b = va_arg(ap, struct bar);
  assert(b.f1 == 1);
  assert(b.f2 == 3);
  assert(b.f3 == 5);
  assert(b.f4 == 7);
  assert(b.f5 == 9);
  
  va_end(ap);

  f.f1++;
  return f;
}

int main() {
  struct foo f = { 1, 2, 3, 4, 5, 6, 7 };
  struct bar b = { 11, 12, 13, 14, 15 };
  struct foo res = test1(-1, f, b);
  assert(res.f1 == 2);
  assert(res.f2 == 2);
  assert(res.f3 == 3);
  assert(res.f4 == 4);
  assert(res.f5 == 5);
  assert(res.f6 == 6);
  assert(res.f7 == 7);
  // check that f was not modified, as it's passed by value
  assert(f.f1 == 1);

  int i = 10;
  long long int l = 1000;
  struct foo f2 = { 10, 20, 30, 40, 50, 60, 70 };
  struct bar b2 = { 1, 3, 5, 7, 9 };
  test2(i, l, i, f, l, b, f2, b2);
}
