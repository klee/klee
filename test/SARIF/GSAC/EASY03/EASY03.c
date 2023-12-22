// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -write-sarifs --use-sym-size-alloc --use-sym-size-li --skip-not-symbolic-objects --posix-runtime --libc=uclibc -cex-cache-validity-cores --output-dir=%t.klee-out %t.bc > %t.log
// RUN: %checker %t.klee-out/report.sarif %S/pattern.sarif

/***************************************************************************************
 *    Title: GSAC
 *    Author: https://github.com/GSACTech
 *    Date: 2023
 *    Code version: 1.0
 *    Availability: https://github.com/GSACTech/contest
 *
 ***************************************************************************************/

#include <stdlib.h>

int main() {
  int *data;
  int size = 5;
  data = malloc(size * sizeof(int)); // Memory allocation
  for (int i = 0; i <= size; i++) {  // Last iteration of this cycle is (size + 1)_th
    // In the last iteration access is out of bounds
    data[i] = i; // buffer-overflow
  }
  free(data);
}
