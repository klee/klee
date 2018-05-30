// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: klee --seed-out=%S/replay_concretize_testcase.ktest --only-replay-seeds %t.bc --sym-stdin 7

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "klee/klee.h"

int main(int argc, char *argv[]) {
  int blah = 0;
  int bar = 0;
  char ar[7];
  klee_make_symbolic(ar, 7, "stdin");
  char c = ar[0];
  if (c == EOF)
    return 1;

  char *d = malloc(c+1);
  memset(d, 0, c+1);
  int i = 0;
  while (i < 7) {
    c = ar[i];
    d[blah] = c;
    blah++;
  }

  return 0;
}
