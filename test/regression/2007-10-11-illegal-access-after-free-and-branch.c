// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --optimize %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.ptr.err

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
  {
    // CHECK: 2007-10-11-illegal-access-after-free-and-branch.c:19: memory error: out of bound pointer
    return buf[2];
  }
  klee_silent_exit(0);
  return 0;
}
