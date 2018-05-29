// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: klee --seed-out=./replay_concretize_testcase.ktest --only-replay-seeds --write-smt2s -libc=uclibc -posix-runtime ./%t.bc --sym-stdin 7

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int blah = 0;
  int bar = 0;
  char c = getchar(); 
  if (c == EOF)
    return 1;

  char *d = malloc(c+1);
  memset(d, 0, c+1);
  while ((c = getchar()) != EOF) {
    d[blah] = c;
    blah++;
  }

  return 0;
}
