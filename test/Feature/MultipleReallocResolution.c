// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc
// RUN: ls %t.klee-out/ | grep .err | wc -l | grep 2
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 2

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

unsigned klee_urange(unsigned start, unsigned end) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x, "x");
  if (x-start>=end-start) klee_silent_exit(0);
  return x;
}

int *make_int(int i, int N) {
  int *x = malloc(sizeof(*x) * N);
  *x = i;
  return x;
}

int main() {
  int *buf[4];

  buf[0] = malloc(sizeof(int)*4);
  buf[1] = malloc(sizeof(int));
  buf[2] = 0; // gets malloc'd
  buf[3] = (int*) 0xdeadbeef; // boom

  buf[0][0] = 10;
  buf[0][1] = 42;
  buf[1][0] = 20;

  int i = klee_urange(0,4);
  int new_size = 2 * sizeof(int) * klee_urange(0,2); // 0 or 8

  // whee, party time, needs to:
  // Fork if size == 0, in which case all buffers get free'd and
  // we will crash in prints below.
  // Fork if buf[s] == 0, in which case the buffer gets malloc'd (for s==2)
  // Fork on other buffers (s in [0,1]) and do realloc
  //   for s==0 this is shrinking
  //   for s==1 this is growing
  buf[i] = realloc(buf[i], new_size);

  if (new_size == 0) {
    assert(!buf[2]); // free(0) is a no-op
    if (i==0) assert(!buf[0] && buf[1]);
    if (i==1) assert(buf[0] && !buf[1]);
    assert(i != 3); // we should have crashed on free of invalid
  } else {
    assert(new_size == sizeof(int)*2);
    assert(buf[0][0] == 10);
    assert(buf[0][1] == 42); // make sure copied
    assert(buf[1][0] == 20);
    if (i==1) { int x = buf[1][1]; } // should be safe
    if (i==2) { int x = buf[2][0]; } // should be safe
    assert(i != 3); // we should have crashed on realloc of invalid
  }

  return 0;
}
