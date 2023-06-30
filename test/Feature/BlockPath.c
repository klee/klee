// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --write-kpaths %t1.bc
// RUN: grep "(path: 0 (main: %0 %5 %6 %8 (abs: %1 %8 %10 %12) %8 %10 %17 %19" %t.klee-out/test000001.kpath
// RUN: grep "(path: 0 (main: %0 %5 %6 %8 (abs: %1 %7 %10 %12) %8 %10 %17 %19" %t.klee-out/test000002.kpath

#include "klee/klee.h"
#include <limits.h>

int abs(int x) {
  if (x >= 0) {
    return x;
  }
  return -x;
}

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");
  int y = abs(x);
  if (y != INT_MIN && y < 0) {
    printf(!" Reached ");
  }
}
