// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc 0.5 > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <assert.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  klee_set_rounding_mode(KLEE_FP_RNA);
  // CHECK: set_rounding_mode_fail_external_call.c:[[@LINE+1]]: Cannot set rounding mode for external call to RNA
  puts("doing external call");
  return 0;
}
