// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -kdalloc -kdalloc-quarantine=1 -output-dir=%t.klee-out %t.bc -exit-on-error >%t.output 2>&1
// RUN: FileCheck %s -input-file=%t.output

#include <stdlib.h>
#include <string.h>

int main() {
  void *ptr = malloc(4096);
  free(ptr);

  // CHECK: use after free
  memset(ptr, 0, 4096);

  return 0;
}
