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

int *foo(int size) {
  int *data = (int *)malloc(size * sizeof(int));
  return data;
}

int main() {
  int size = 5;
  int *first_data = foo(size++);  // Call of 'foo(5)'
  int *second_data = foo(size--); // Call of 'foo(6)'

  for (int i = 0; i <= size; i++) {
    // Length of 'first_data' is 5 and 'size' also is 5
    first_data[i] = i; // buffer-overflow
  }
  for (int i = 0; i <= size; i++) {
    // Length of 'second_data' is 6
    second_data[i] = i;
  }
  free(first_data);
  free(second_data);
}
