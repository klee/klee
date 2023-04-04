// This test checks that __memcpy_chk is properly handled

// RUN: %clang %s -emit-llvm -O2 -g -c -D_FORTIFY_SOURCE=1 -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t2.bc

// RUN: %clang %s -emit-llvm -O2 -g -c -D_FORTIFY_SOURCE=2 -o %t3.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t3.bc

#include "klee/klee.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main() {
#define N 4
  char *s = malloc(N);
  klee_make_symbolic(s, N, "s");
  s[N - 1] = '\0';
  char *d = malloc(N);
  memcpy(d, s, N);

  int i;
  for (i = 0; i < N; i++)
    assert(s[i] == d[i]);
}
