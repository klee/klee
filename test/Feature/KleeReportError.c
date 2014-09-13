// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors %t2.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .my.err | wc -l | grep 2
#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {
  int x, y, *p = 0;
  
  klee_make_symbolic(&x, sizeof x);
  klee_make_symbolic(&y, sizeof y);

  if (x)
    fprintf(stderr, "x\n");
  else fprintf(stderr, "!x\n");

  if (y) {
    fprintf(stderr, "My error\n");
    // CHECK: KleeReportError.c:23: My error
    // CHECK: KleeReportError.c:23: My error
    // FIXME: Use FileCheck's relative line number syntax
    klee_report_error(__FILE__, __LINE__, "My error", "my.err");
  }

  return 0;
}
