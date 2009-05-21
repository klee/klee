// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: %klee --optimize %t1.bc
// RUN: test -f klee-last/test000001.ptr.err

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char **argv) {
  unsigned char *buf = malloc(3);
  klee_make_symbolic(buf, 3);
  if (buf[0]>4) klee_silent_exit(0);
  unsigned char x = buf[1];
  free(buf);
  if (x)
    return buf[2];
  klee_silent_exit(0);
  return 0;
}
