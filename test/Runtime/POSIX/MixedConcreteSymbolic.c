// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=all --exit-on-error --libc=uclibc --posix-runtime %t.bc --sym-stdin 10  2>%t.log | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv) {
  char buf[32];

  fread(buf, 1, 4, stdin);
  klee_assume(buf[0] == 'a');
  klee_assume(buf[1] > 'a');
  klee_assume(buf[1] < 'z');
  klee_assume(buf[2] == '\n');
  klee_assume(buf[3] == '\0');
  fwrite(buf, 1, 4, stdout);
  //CHECK: a{{[b-y]}}

  return 0;
}
