// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: test -f %t.klee-out/test000004.ktest
// RUN: test -f %t.klee-out/test000005.ktest
// RUN: test -f %t.klee-out/test000006.ktest
// RUN: not test -f %t.klee-out/test000007.ktest

#include "klee/klee.h"
#include <stdio.h>

char simple_puts(char c) {
  if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
    char a[] = "Vowel";
    puts(a);
    return 'V';
  } else {
    char a[] = "Consonant";
    puts(a);
    return 'C';
  }
}

int main() {
  char c;
  klee_make_symbolic(&c, sizeof(char), "c");
  char d = simple_puts(c);
  return 0;
}
