/* The test is flaky and the feature isn't importatnt now */
// REQUIRES: not-ubsan, not-darwin
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out %t.entrypoints
// RUN: echo sign_sum >> %t.entrypoints
// RUN: echo comparison >> %t.entrypoints
// RUN: echo segment_intersection >> %t.entrypoints
// RUN: echo main >> %t.entrypoints
// RUN: %klee --output-dir=%t.klee-out --entry-point=main --interactive --entrypoints-file=%t.entrypoints %t.bc

// RUN: test -f %t.klee-out/sign_sum/test000001.ktest
// RUN: test -f %t.klee-out/sign_sum/test000002.ktest
// RUN: test -f %t.klee-out/sign_sum/test000003.ktest
// RUN: not test -f %t.klee-out/sign_sum/test000004.ktest

// RUN: test -f %t.klee-out/comparison/test000001.ktest
// RUN: test -f %t.klee-out/comparison/test000002.ktest
// RUN: not test -f %t.klee-out/comparison/test000003.ktest

// RUN: test -f %t.klee-out/segment_intersection/test000001.ktest
// RUN: test -f %t.klee-out/segment_intersection/test000002.ktest
// RUN: test -f %t.klee-out/segment_intersection/test000003.ktest
// RUN: test -f %t.klee-out/segment_intersection/test000004.ktest
// RUN: test -f %t.klee-out/segment_intersection/test000005.ktest
// RUN: test -f %t.klee-out/segment_intersection/test000006.ktest
// RUN: test -f %t.klee-out/segment_intersection/test000007.ktest
// RUN: test -f %t.klee-out/segment_intersection/test000008.ktest
// RUN: test -f %t.klee-out/segment_intersection/test000009.ktest
// RUN: not test -f %t.klee-out/segment_intersection/test000010.ktest

// RUN: test -f %t.klee-out/main/test000001.ktest
// RUN: not test -f %t.klee-out/main/test000002.ktest

#include "klee/klee.h"
#include <stdio.h>

int sign_sum() {
  int x, y;
  klee_make_symbolic(&x, sizeof(int), "x");
  klee_make_symbolic(&y, sizeof(int), "y");

  int c = 0;
  if (x + y > 0) {
    c = 1;
  } else if (x + y < 0) {
    c = -1;
  } else {
    c = 0;
  }
  return 0;
}

int comparison() {
  int x, y;
  klee_make_symbolic(&x, sizeof(int), "x");
  klee_make_symbolic(&y, sizeof(int), "y");

  if (x < y) {
    return 1;
  } else {
    return 0;
  }
}

int segment_intersection() {
  int l, r, A, B;
  klee_make_symbolic(&l, sizeof(int), "l");
  klee_make_symbolic(&r, sizeof(int), "r");
  klee_make_symbolic(&A, sizeof(int), "A");
  klee_make_symbolic(&B, sizeof(int), "B");

  if (l > r || A > B) {
    return -1;
  }

  if (l > B || r < A) {
    return 0;
  }

  if (A <= l && r <= B) {
    return r - l;
  }

  if (l <= A && B <= r) {
    return B - A;
  }

  return (r <= B ? r : B) - (l >= A ? l : A);
}

int main() {
  return 0;
}
