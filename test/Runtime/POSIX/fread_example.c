// REQUIRES: geq-llvm-10.0
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: not test -f %t.klee-out/test000004.ktest

#include <stdio.h>

int main() {
  int arrA[4];
  int arrB[4];
  fread(arrA, sizeof(int), 4, stdin);
  fread(arrB, sizeof(int), 4, stdin);

  int sumA = 0, sumB = 0;
  for (int i = 0; i < 4; i++) {
    sumA += arrA[i];
    sumB += arrB[i];
  }
  if (sumA == sumB) {
    return 0;
  } else if (sumA > sumB) {
    return 1;
  } else {
    return -1;
  }
}
