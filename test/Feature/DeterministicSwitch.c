// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -debug-print-instructions=all:stderr --output-dir=%t.klee-out --switch-type=internal --search=dfs %t.bc >%t.switch.log 2>&1
// RUN: FileCheck %s -input-file=%t.switch.log -check-prefix=CHECK-DFS
// RUN: rm -rf %t.klee-out
// RUN: %klee -debug-print-instructions=all:stderr --output-dir=%t.klee-out --switch-type=internal --search=bfs %t.bc >%t.switch.log 2>&1
// RUN: FileCheck %s -input-file=%t.switch.log -check-prefix=CHECK-BFS

#include "klee/klee.h"
#include <stdio.h>

int main(int argc, char **argv) {
  char c;

  klee_make_symbolic(&c, sizeof(c), "index");

  switch (c) {
  case '\t':
    printf("tab\n");
    break;
  case ' ':
    printf("space\n");
    break;
  default:
    printf("default\n");
    break;
  }

  // CHECK-DFS: default
  // CHECK-DFS: tab
  // CHECK-DFS: space

  // CHECK-BFS: space
  // CHECK-BFS: tab
  // CHECK-BFS: default

  return 0;
}
