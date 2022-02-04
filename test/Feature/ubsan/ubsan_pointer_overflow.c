// RUN: %clang %s -fsanitize=pointer-overflow -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s %c_prefixes_10

// RUN: %clangxx %s -std=c++11 -fsanitize=pointer-overflow -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s %cpp_prefixes_10

#include "klee/klee.h"
#include <stdio.h>

int main() {
  size_t base1, base2, base3;
  klee_make_symbolic(&base1, sizeof base1, "base1");
  klee_make_symbolic(&base2, sizeof base2, "base2");
  klee_make_symbolic(&base3, sizeof base3, "base3");
  klee_assume(base3 != 0);

  char *base1_ptr = (char *)base1;
  char *base2_ptr = (char *)base2;
  char *base3_ptr = (char *)base3;
  volatile char *result_ptr;

  // CHECK-C-10: ubsan_pointer_overflow.c:[[@LINE+1]]: applying zero offset to null pointer
  result_ptr = base1_ptr + 0;

  // CHECK-10: ubsan_pointer_overflow.c:[[@LINE+2]]: applying non-zero offset 1 to null pointer
  // CHECK: ubsan_pointer_overflow.c:[[@LINE+1]]: applying non-zero offset to non-null pointer 0x{{.*}} produced null pointer
  result_ptr = base2_ptr + 1;

  // CHECK-10: ubsan_pointer_overflow.c:[[@LINE+1]]: applying non-zero offset to non-null pointer 0x{{.*}} produced null pointer
  result_ptr = base3_ptr - 1;

  char c;
  unsigned long long offset = -1;
  // CHECK-10: ubsan_pointer_overflow.c:[[@LINE+2]]: pointer arithmetic with base 0x{{.*}} overflowed to 0x{{.*}}
  // CHECK: ubsan_pointer_overflow.c:[[@LINE+1]]: pointer arithmetic with base 0x{{.*}} overflowed to 0x{{.*}}
  result_ptr = &c + offset;

  return 0;
}
