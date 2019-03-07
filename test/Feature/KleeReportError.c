// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors %t2.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .my.err | wc -l | grep 2
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv) {
  int x, y, *p = 0;

  klee_make_symbolic(&x, sizeof x, "x");
  klee_make_symbolic(&y, sizeof y, "y");

  if (x)
    fprintf(stderr, "x\n");
  else
    fprintf(stderr, "!x\n");

  if (y) {
    fprintf(stderr, "My error\n");
    // CHECK: KleeReportError.c:[[@LINE+2]]: My error
    // CHECK: KleeReportError.c:[[@LINE+1]]: My error
    klee_report_error(__FILE__, __LINE__, "My error", "my.err");
  }

  return 0;
}
