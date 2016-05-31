// RUN: %llvmgcc -emit-llvm -c -g -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --zest --output-level=error %t1.bc a
// RUN: test -f %t.klee-out/test000001.ptr.err

#include <stdio.h>

// check that zest doesn't concretize switch variables
// when the default path is concrete
int main(int argc, char **argv) {
  int a[100] = {0};

  char opt = argv[1][0];
  switch (opt) {
  case '1':
    return 2;
  case '2':
    return 2;
  default:
    return a[opt];
  }
}
