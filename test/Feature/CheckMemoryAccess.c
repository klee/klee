// RUN: %llvmgcc -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc > %t.log
// RUN: grep -q "good" %t.log
// RUN: not grep -q "bad" %t.log

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
  char buf[4];

  klee_check_memory_access(&buf, 1);
  printf("good\n");
  if (klee_range(0, 2, "range1")) {
    klee_check_memory_access(0, 1);
    printf("null pointer deref: bad\n");
  }

  if (klee_range(0, 2, "range2")) {
    klee_check_memory_access(buf, 5);
    printf("oversize access: bad\n");
  }

  return 0;
}
