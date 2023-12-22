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

/*
 * CVE-2023-0819
 */

#include <stdlib.h>

int main() {
  int *data;
  int size = 5;
  data = malloc(size * sizeof(int)); // Allocated 20 bytes of memory
  // Length of 'data' is 5
  for (int i = 0; i < size; i++) {
    data[i] = i;
  }
  if (data[0]) { // Condition is always false
    data[5] = data[4];
  }
  if (data[4]) {
    data[5] = data[4]; // buffer-overflow
  }
  free(data);
}
