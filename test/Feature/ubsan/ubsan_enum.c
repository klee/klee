// RUN: %clangxx %s -fsanitize=enum -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

enum E { a = 1 } e;

int main() {
  unsigned char x;
  volatile bool result;

  klee_make_symbolic(&x, sizeof(x), "x");
  for (unsigned char *p = (unsigned char *)&e; p != (unsigned char *)(&e + 1); ++p)
    *p = x;

  // CHECK: ubsan_enum.c:[[@LINE+1]]: load invalid value
  result = (int)e != -1;

  return 0;
}
