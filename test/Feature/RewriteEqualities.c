// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs --write-kqueries --rewrite-equalities=false %t.bc
// RUN: grep "N0:(Read w8 2 x)" %t.klee-out/test000003.kquery
// RUN: grep "N0)" %t.klee-out/test000003.kquery
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs --write-kqueries --rewrite-equalities %t.bc
// RUN: FileCheck -input-file=%t.klee-out/test000003.kquery %s

#include "klee/klee.h"

#include <stdio.h>

int run(unsigned char * x, unsigned char * y) {
  y[6] = 15;

  if(x[2] >= 10){
    return 1;
  }

  if(y[x[2]] < 11){
    if(x[2] == 8){
      // CHECK:      (query [(Eq 8 (Read w8 2 x))]
      // CHECK-NEXT: false)
      return 2;
    } else{
      return 3;
    }
  } else{
    return 4;
  }
}

unsigned char y[255];

int main() {
  unsigned char x[4];
  klee_make_symbolic(&x, sizeof x, "x");

  return run(x, y);
}
