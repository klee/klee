// RUN: %llvmgcc %s -emit-llvm -g -c -o %t2.bc
// RUN: %klee --exit-on-error --write-cov %t2.bc
// RUN: grep -c WriteCov.c:15  klee-last/test000001.cov klee-last/test000002.cov  >%t3.txt
// RUN: grep -c WriteCov.c:17  klee-last/test000001.cov klee-last/test000002.cov >>%t3.txt
// RUN: grep klee-last/test000001.cov:0 %t3.txt
// RUN: grep klee-last/test000001.cov:1 %t3.txt
// RUN: grep klee-last/test000002.cov:0 %t3.txt
// RUN: grep klee-last/test000002.cov:1 %t3.txt

#include <assert.h>
#include <stdio.h>

int main() {
  if (klee_range(0,2, "range")) {
    assert(__LINE__ == 15); printf("__LINE__ = %d\n", __LINE__);
  } else {
    assert(__LINE__ == 17); printf("__LINE__ = %d\n", __LINE__);
  }
  return 0;
}
