// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=other_main %t.bc > %t.log
// RUN: grep "Hello World" %t.log

#include <stdio.h>

int other_main() {
  printf("Hello World\n");
  return 0;
}
