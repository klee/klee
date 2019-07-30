// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --search=bfs %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --search=dfs %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --search=nurs:covnew %t.bc 2>&1 | FileCheck %s

// CHECK: open merge:
// CHECK: generated tests = 2{{$}}

// This test will not merge because we cannot merge states when they allocated memory.

#include "klee/klee.h"

int main(int argc, char **args) {

  int* arr = 0;
  int a = 0;

  klee_make_symbolic(&a, sizeof(a), "a");

  klee_open_merge();
  if (a == 0) {
    arr = (int*) malloc(7 * sizeof(int));
    arr[0] = 7;
  } else {
    arr = (int*) malloc(8 * sizeof(int));
    arr[0] = 8;
  }
  klee_close_merge();
  a = arr[0];
  free(arr);

  return a;
}
