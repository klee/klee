// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs --write-pcs --rewrite-equalities=false %t.bc
// RUN: grep "N0:(Read w8 2 x)" %t.klee-out/test000003.pc
// RUN: grep "N0)" %t.klee-out/test000003.pc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs --write-pcs --rewrite-equalities %t.bc
// RUN: grep "(Read w8 8 const_arr1)" %t.klee-out/test000003.pc

#include <stdio.h>
#include <klee/klee.h>

int run(unsigned char * x, unsigned char * y) {
  y[6] = 15;

  if(x[2] >= 10){
    return 1;
  }

  if(y[x[2]] < 11){
    if(x[2] == 8){
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
