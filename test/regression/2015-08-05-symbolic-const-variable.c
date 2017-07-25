// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -exit-on-error %t.bc 2> %t.log

#include "klee/klee.h"

const int c = 23;

int main(int argc, char** argv) {

  klee_make_symbolic(&c, sizeof(c), "c");

  if(c > 20)
	  return 0;
  else
	  return 1;
}
