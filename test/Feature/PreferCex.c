// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --prefer-cex %t1.bc
// RUN: ktest-tool %t.klee-out/test000001.ktest | FileCheck %s

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
  klee_prefer_cex(buf, buf[3]=='\0');

  return 0;
}
