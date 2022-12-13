// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: not test -f %t.klee-out/test000004.ktest

#include "klee/klee.h"
#include <stdio.h>

char simple_fwrite(int x) {
  if (x > 0) {
    char a[] = "Positive";
    fwrite(a, sizeof(char), 8, stdout);
    return 'P';
  } else if (x < 0) {
    char a[] = "Negative";
    fwrite(a, sizeof(char), 8, stdout);
    return 'N';
  } else {
    char a[] = "Zero";
    fwrite(a, sizeof(char), 4, stdout);
    return 'Z';
  }
}

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(int), "x");
  char c = simple_fwrite(x);
  return 0;
}
