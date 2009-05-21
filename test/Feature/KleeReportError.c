// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: %klee --emit-all-errors %t2.bc > %t3.log
// RUN: ls klee-last/ | grep .my.err | wc -l | grep 2
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
    klee_report_error(__FILE__, __LINE__, "My error", "my.err");
  }

  return 0;
}
