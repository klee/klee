// RUN: %llvmgcc -emit-llvm -c -g -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --zest --output-level=error --use-symbex=1 --symbex-for=2 --use-zest-search --every-access %t1.bc a
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 2

#include <stdio.h>

int main(int argc, char **argv) {
  int a[10] = {0};

  char opt = argv[1][0];
  switch (opt) {
  case 'a':
    return 2;
  case 'b':
    return a[opt];
  default:
    return a[20];
  }
}
