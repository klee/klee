// RUN: %llvmgcc %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --allow-external-sym-calls --switch-type=internal %t.bc
// RUN: not test -f %t.klee-out/test000010.ktest

// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --allow-external-sym-calls --switch-type=simple %t.bc
// RUN: test -f %t.klee-out/test000010.ktest

#include <stdio.h>

int main(int argc, char **argv) {
  int c = klee_range(0, 256, "range");
  
  switch(c) {
  case 0: printf("0\n"); break;
  case 10: printf("10\n"); break;
  case 16: printf("16\n"); break;
  case 17: printf("17\n"); break;
  case 18: printf("18\n"); break;
  case 19: printf("19\n"); break;
#define C(x) case x: case x+1: case x+2: case x+3
#define C2(x) C(x): C(x+4): C(x+8): C(x+12)
#define C3(x) C2(x): C2(x+16): C2(x+32): C2(x+48)
  C3(128):
    printf("bignums: %d\n", c); break;
  default: 
    printf("default\n");
    break;
  }

  return 0;
}
