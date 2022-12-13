// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: not test -f %t.klee-out/test000004.ktest

#include "klee/klee.h"
#include <stdio.h>

char simple_putc(int x, int y) {
  if (x + y > 0) {
    putc('1', stdout);
    return '1';
  } else if (x + y < 0) {
    putc('2', stdout);
    return '2';
  } else {
    putc('0', stdout);
    return '0';
  }
}

int main() {
  int x, y;
  klee_make_symbolic(&x, sizeof(int), "x");
  klee_make_symbolic(&y, sizeof(int), "y");
  char c = simple_putc(x, y);
  return 0;
}
