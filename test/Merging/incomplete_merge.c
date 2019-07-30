// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --use-incomplete-merge --debug-log-incomplete-merge --search=nurs:covnew --use-batching-search %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --use-incomplete-merge --debug-log-incomplete-merge --search=bfs --use-batching-search %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --use-incomplete-merge --debug-log-incomplete-merge --search=dfs --use-batching-search %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --use-incomplete-merge --debug-log-incomplete-merge --search=nurs:covnew %t.bc 2>&1 | FileCheck %s

// CHECK: open merge:
// 5 close merges
// CHECK: close merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: close merge:

// This test merges branches with vastly different instruction counts.
// The incomplete merging heuristic merges preemptively if a branch takes too long.
// It might occur that the random branch selection completes the heavy branch first,
// which results in the branches being merged completely.

#include "klee/klee.h"

int main(int argc, char **args) {

  int x;
  int a;
  int foo = 0;

  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&a, sizeof(a), "a");

  klee_open_merge();
  if (a == 0) {
    klee_open_merge();

    for (int i = 0; i < 5; ++i){
      foo += 2;
    }
    if (x == 1) {
      foo += 5;
    } else if (x == 2) {
      for (int i = 0; i < 10; ++i) {
        foo += foo;
      }
    } else {
      foo += 7;
    }

    klee_close_merge();
  } else if (a == 1) {
    foo = 4;
  } else {
    foo = 3;
  }
  klee_close_merge();

  return foo;
}
