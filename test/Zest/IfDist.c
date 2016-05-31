// RUN: %llvmgcc -emit-llvm -c -g -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --zest --output-level=error --use-symbex=1 --symbex-for=5 --use-zest-search %t1.bc z
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 1
// RUN: grep "Current searcherdistance 0" %t.klee-out/test000001.ptr.err

#include <stdio.h>

int main(int argc, char **argv) {
  int a[10] = {0};

  char opt = argv[1][0];

  int index = 9;

  if (opt == 'a')
    index += 0;
  else if (opt == 'b')
    index += 2;

  return a[index];
}
