// RUN: echo "x" > %t1.res
// RUN: echo "x" >> %t1.res
// RUN: echo "x" >> %t1.res
// RUN: echo "x" >> %t1.res
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc > %t1.log
// RUN: diff %t1.res %t1.log

#include <stdio.h>

unsigned klee_urange(unsigned start, unsigned end) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x, "x");
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
  int i,s,t;

  for (i=0; i<4; i++)
    buf[i] = make_int((i+1)*2);

  s = klee_urange(0,4);

  *buf[s] = 5;

  if ((*buf[0] + *buf[1] + *buf[2] + *buf[3]) == 17)
    if (s!=3)
      abort();

  printf("x\n");
  fflush(stdout);

  return 0;
}
