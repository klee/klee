// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=random-state %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=nurs:depth %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=nurs:qc %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-batching-search %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-batching-search --search=random-state %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-batching-search --search=nurs:depth %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-batching-search --search=nurs:qc %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=random-path --search=nurs:qc %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --search=dfs --debug-log-merge --debug-log-state-merge %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --use-batching-search --search=dfs %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --use-batching-search --search=random-state %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --use-batching-search --search=nurs:depth %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --use-batching-search --search=nurs:qc %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-iterative-deepening-time-search --use-batching-search %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-iterative-deepening-time-search --use-batching-search --search=random-state %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-iterative-deepening-time-search --use-batching-search --search=nurs:depth %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-iterative-deepening-time-search --use-batching-search --search=nurs:qc %t2.bc


/* this test is basically just for coverage and doesn't really do any
   correctness check (aside from testing that the various combinations
   don't crash) */

#include <stdlib.h>

int validate(char *buf, int N) {

  int i;

  for (i=0; i<N; i++) {
    if (buf[i]==0) {
      klee_merge();
      return 0;
    }
  }
  
  klee_merge();
  return 1;
}

#ifndef SYMBOLIC_SIZE
#define SYMBOLIC_SIZE 15
#endif
int main(int argc, char **argv) {
  int N = SYMBOLIC_SIZE;
  unsigned char *buf = malloc(N);
  int i;
  
  klee_make_symbolic(buf, N);
  if (validate(buf, N))
    return buf[0];
  return 0;
}
