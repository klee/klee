// RUN: %llvmgcc -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --zest %t1.bc
// RUN: test -f %t.klee-out/test000001.ptr.err

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int v[100] = {0};
  int idx = 0;

  klee_make_symbolic(&idx, sizeof idx, "idx");

  return v[idx];
}
