// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-sym-size-alloc --skip-not-lazy-initialized %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

struct field_range_pair {
  uintmax_t lo;
  uintmax_t hi;
};

static int
compare_ranges(const void *a, const void *b) {
  struct field_range_pair const *ap = a, *bp = b;
  // CHECK: SegmentComparator.c:[[@LINE+1]]: memory error: null pointer exception
  return (ap->lo > bp->lo) - (ap->hi < bp->hi);
}

int main() {
  void *a;
  klee_make_symbolic(&a, sizeof(a), "a");
  void *b;
  klee_make_symbolic(&b, sizeof(b), "b");

  int res = compare_ranges(a, b);

  // CHECK-DAG: Less
  // CHECK-DAG: Equals
  // CHECK-DAG: Greater
  if (res == -1) {
    printf("Less\n");
  } else if (res == 0) {
    printf("Equals\n");
  } else {
    printf("Greater\n");
  }
}
