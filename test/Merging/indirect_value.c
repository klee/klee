// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --search=nurs:covnew --use-batching-search %t.bc 2>&1 | FileCheck %s

// CHECK: generated tests = 2{{$}}

#include "klee/klee.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {

  int sym = klee_int("sym");
  int* heap_int = calloc(1, sizeof(*heap_int));

  klee_open_merge();

  if(sym != 0) {
    *heap_int = 1;
  }

  klee_close_merge();

  klee_print_expr("*heap_int: ", *heap_int);
  if(*heap_int != 0) {
    printf("true\n");
  } else {
    printf("false\n");
  }

  return 0;
}
