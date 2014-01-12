// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --exit-on-error %t1.bc
// RUN: ktest-tool %T/klee-last/test000001.ktest | FileCheck %s

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
  char buf[4];

  klee_make_symbolic(buf, sizeof buf);
  // CHECK: Hi\x00\x00
  klee_prefer_cex(buf, buf[0]=='H');
  klee_prefer_cex(buf, buf[1]=='i');
  klee_prefer_cex(buf, buf[2]=='\0');

  return 0;
}
