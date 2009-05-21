// RUN: %llvmgcc %s --emit-llvm -O0 -c -o %t2.bc
// RUN: %klee %t2.bc
// RUN: %klee --use-random-search %t2.bc
// RUN: %klee --use-non-uniform-random-search %t2.bc
// RUN: %klee --use-non-uniform-random-search --weight-type=query-cost %t2.bc
// RUN: %klee --use-batching-search %t2.bc
// RUN: %klee --use-batching-search --use-random-search %t2.bc
// RUN: %klee --use-batching-search --use-non-uniform-random-search %t2.bc
// RUN: %klee --use-batching-search --use-non-uniform-random-search --weight-type=query-cost %t2.bc
// RUN: %klee --use-merge --debug-log-merge --debug-log-state-merge %t2.bc
// RUN: %klee --use-merge --use-batching-search %t2.bc
// RUN: %klee --use-merge --use-batching-search --use-random-search %t2.bc
// RUN: %klee --use-merge --use-batching-search --use-non-uniform-random-search %t2.bc
// RUN: %klee --use-merge --use-batching-search --use-non-uniform-random-search --weight-type=query-cost %t2.bc
// RUN: %klee --use-iterative-deepening-time-search --use-batching-search %t2.bc
// RUN: %klee --use-iterative-deepening-time-search --use-batching-search --use-random-search %t2.bc
// RUN: %klee --use-iterative-deepening-time-search --use-batching-search --use-non-uniform-random-search %t2.bc
// RUN: %klee --use-iterative-deepening-time-search --use-batching-search --use-non-uniform-random-search --weight-type=query-cost %t2.bc


/* this test is basically just for coverage and doesn't really do any
   correctness check (aside from testing that the various combinations
   don't crash) */

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
