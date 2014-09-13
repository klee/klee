// RUN: %llvmgcc -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc > %t.log
// RUN: sort %t.log | uniq -c > %t.uniq.log
// RUN: grep "1 A" %t.uniq.log
// RUN: grep "1 B" %t.uniq.log
// RUN: grep "1 C" %t.uniq.log

#include <stdio.h>

int main() {
  klee_set_forking(0);
  
  if (klee_range(0, 2, "range")) {
    printf("A\n");
  } else {
    printf("A\n");
  }

  klee_set_forking(1);

  if (klee_range(0, 2, "range")) {
    printf("B\n");
  } else {
    printf("C\n");
  }

  return 0;
}
