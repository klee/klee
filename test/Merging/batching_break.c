// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --search=nurs:covnew %t.bc 2>&1 | FileCheck %s

// CHECK: open merge:
// CHECK: close merge:
// CHECK: KLEE: done: generated tests = 3{{$}}

#include "klee/klee.h"

int main(int argc, char** args){

  int x;
  int p;
  int i;

  klee_make_symbolic(&x, sizeof(x), "x");
  x = x % 20;

  klee_open_merge();
  for (i = 0; i < x; ++i){
    if (x % 3 == 0){
      klee_close_merge();
      if (x > 10){
        return 1;
      } else {
        return 2;
      }
    }
  }
  klee_close_merge();

  klee_open_merge();
  if (x > 10){
    p = 1;
  } else {
    p = 2;
  }
  klee_close_merge();
  return p;

}
