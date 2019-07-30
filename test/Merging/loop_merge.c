// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --search=bfs %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --search=dfs %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --search=nurs:covnew %t.bc 2>&1 | FileCheck %s

// CHECK: open merge:
// There will be 20 'close merge' statements. Only checking a few, the generated
// test count will confirm that the merge was closed correctly
// CHECK: close merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: generated tests = 2{{$}}

#include "klee/klee.h"

int main(int argc, char** args){

  int x;
  int i;

  klee_make_symbolic(&x, sizeof(x), "x");
  x = x % 20;

  klee_open_merge();
  for (i = 0; i < x; ++i){
    if (x % 3 == 0){
      klee_close_merge();
      return 1;
    }
  }
  klee_close_merge();

  return 0;
}
