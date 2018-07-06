// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -search=dfs --output-dir=%t.klee-out %t1.bc | FileCheck %s

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <klee/klee.h>

void start(int x) {
  printf("START\n");
  if (x == 53)
    exit(1);
}

void __attribute__ ((noinline)) end(int status) {
  klee_alias_function("exit", "exit");
  printf("END: status = %d\n", status);
  exit(status);
}


int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  klee_alias_function("exit", "end");
  // CHECK: START
  // First path (x != 53)
  // CHECK: END: status = 0
  // Second path (x == 53)
  // CHECK: END: status = 1
  start(x);
  end(0);
}
