// RUN: %clang %s -emit-llvm -g -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --write-cov %t2.bc
// RUN: grep -c WriteCov.c:16  %t.klee-out/test000001.cov %t.klee-out/test000002.cov  >%t3.txt
// RUN: grep -c WriteCov.c:18  %t.klee-out/test000001.cov %t.klee-out/test000002.cov >>%t3.txt
// RUN: grep %t.klee-out/test000001.cov:0 %t3.txt
// RUN: grep %t.klee-out/test000001.cov:1 %t3.txt
// RUN: grep %t.klee-out/test000002.cov:0 %t3.txt
// RUN: grep %t.klee-out/test000002.cov:1 %t3.txt

#include <assert.h>
#include <stdio.h>

int main() {
  if (klee_range(0,2, "range")) {
    assert(__LINE__ == 16); printf("__LINE__ = %d\n", __LINE__);
  } else {
    assert(__LINE__ == 18); printf("__LINE__ = %d\n", __LINE__);
  }
  return 0;
}
