// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -function-alias=exit:end %t.bc 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// CHECK: KLEE: function-alias: replaced @exit with @end

void start(int x) {
  // CHECK: START
  printf("START\n");
  if (x == 53)
    // CHECK: END: status = 1
    exit(1);
}

void __attribute__ ((noinline)) end(int status) {
  printf("END: status = %d\n", status);
  klee_silent_exit(status);
}

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  start(x);
  // CHECK: END: status = 0
  end(0);
}
