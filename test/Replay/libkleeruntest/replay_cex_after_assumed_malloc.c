// RUN: %clang %s -S -emit-llvm -g -c -o %t.ll
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.ll
// KLEE just must not fail
#include "klee/klee.h"
#include <stdlib.h>

int main() {
  char i;
  char *p;
  char *q;
  klee_make_symbolic(&i, sizeof(i), "i");
  klee_make_symbolic(&p, sizeof(p), "p");

  if (i) {}

  q = malloc(sizeof (char));
  klee_assume(p == q);
  klee_make_symbolic(p, sizeof (char), "p[0]");

  char condition = (*p);
  if (*p) condition = 1;
  klee_prefer_cex(&i, condition);
  if (i+5) {}
  return 0;
}
