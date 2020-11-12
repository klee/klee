// This test checks that __strcat_chk is properly handled when klee-libc is used

// RUN: %clang %s -emit-llvm -O2 -g -c -D_FORTIFY_SOURCE=1 -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t2.bc

// RUN: %clang %s -emit-llvm -O2 -g -c -D_FORTIFY_SOURCE=2 -o %t3.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t3.bc

#include "klee/klee.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main() {
#define N 7
  int i;
  char *s = malloc(N);
  klee_make_symbolic(s, N, "s");
  s[N - 1] = '\0';

  char *d = malloc(N + 4);
  d[0] = 'a';
  d[1] = 'b';
  d[2] = 'c';
  d[3] = '\0';
  strcat(d, s);

  assert(!strcmp(s, d + 3));
}
