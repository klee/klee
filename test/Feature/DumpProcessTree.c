// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee  -output-exec-tree --output-dir=%t.klee-out  %t.bc 2> %t.log
// RUN: FileCheck -check-prefix=CHECK-CSV -input-file=%t.klee-out/ptree00000000.csv %s

#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

int get_sign(int x) {
  if (x == 0)
    return 0;

  if (x < 0)
    return -1;
  else
    return 1;
}

int main() {
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  get_sign(a);
  return 0;
}
// Check we find a line with the expected format
// CHECK-CSV: parent,child,location
