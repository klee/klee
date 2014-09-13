// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors %t1.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 4
// RUN: ls %t.klee-out/ | grep .err | wc -l | grep 3

#include <stdlib.h>
#include <stdio.h>

unsigned klee_urange(unsigned start, unsigned end) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x);
  if (x-start>=end-start) klee_silent_exit(0);
  return x;
}

int *make_int(int i) {
  int *x = malloc(sizeof(*x));
  *x = i;
  return x;
}

int main() {
  int *buf[4];
  int i,s;

  for (i=0; i<3; i++)
    buf[i] = make_int(i);
  buf[3] = 0;

  s = klee_urange(0,4);

  free(buf[s]);

  // CHECK: MultipleFreeResolution.c:40: memory error: out of bound pointer
  // CHECK: MultipleFreeResolution.c:40: memory error: out of bound pointer
  // CHECK: MultipleFreeResolution.c:40: memory error: out of bound pointer
  // FIXME: Use FileCheck's relative line numbers
  for (i=0; i<3; i++) {
    printf("*buf[%d] = %d\n", i, *buf[i]);
  }

  return 0;
}
// CHECK: KLEE: done: generated tests = 4
