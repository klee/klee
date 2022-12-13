// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64 --sym-files 1 64
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: test -f %t.klee-out/test000004.ktest
// RUN: test -f %t.klee-out/test000005.ktest
// RUN: test -f %t.klee-out/test000006.ktest
// RUN: test -f %t.klee-out/test000007.ktest
// RUN: test -f %t.klee-out/test000008.ktest
// RUN: test -f %t.klee-out/test000009.ktest
// RUN: test -f %t.klee-out/test000010.ktest
// RUN: test -f %t.klee-out/test000011.ktest
// RUN: test -f %t.klee-out/test000012.ktest
// RUN: not test -f %t.klee-out/test000013.ktest

#include "klee/klee.h"
#include <stdio.h>

char simple_fputs(char c) {
  FILE *fA = fopen("A", "w");
  if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
    char a[] = "Vowel";
    fputs("Vowel", fA);
    return 'V';
  } else {
    char a[] = "Consonant";
    fputs("Consonant", fA);
    return 'C';
  }
}

int main() {
  char c;
  klee_make_symbolic(&c, sizeof(char), "c");
  char d = simple_fputs(c);
  return 0;
}
