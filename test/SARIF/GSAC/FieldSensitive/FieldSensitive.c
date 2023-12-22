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

struct Data {
  int *first_data;
  int *second_data;
};

void foo(struct Data *data, int size) {
  data->second_data = (int *)malloc(size * sizeof(int)); // Allocated 20 bytes of memory
  size++;
  data->first_data = (int *)malloc(size * sizeof(int)); // Allocated 24 bytes of memory
}

int main() {
  int size = 5;
  struct Data data;
  foo(&data, size);
  for (int i = 0; i <= size; i++) {
    data.first_data[i] = i;
  }
  // Length of 'second_data' is 5 and 'size' also is 5
  for (int i = 0; i <= size; i++) {
    data.second_data[i] = i; // buffer-overflow
  }
  free(data.first_data);
  free(data.second_data);
}
