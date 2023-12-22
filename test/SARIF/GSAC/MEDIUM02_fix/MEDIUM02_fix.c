// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -write-sarifs --use-sym-size-alloc --use-sym-size-li --skip-not-symbolic-objects --posix-runtime --libc=uclibc -cex-cache-validity-cores --output-dir=%t.klee-out %t.bc > %t.log
// RUN: test ! -f %t.klee-out/report.sarif

/***************************************************************************************
 *    Title: GSAC
 *    Author: https://github.com/GSACTech
 *    Date: 2023
 *    Code version: 1.0
 *    Availability: https://github.com/GSACTech/contest
 *
 ***************************************************************************************/

/*
 * Based on CVE-2022-3077 (fixed)
 */

#include <stdlib.h>
#include <string.h>

void foo(int *data) {
  unsigned char buffer[32 + 16];
  if (data[0] > 12)
    return;

  memcpy(&buffer[1], &data[1], data[0]);
}

int main() {
  int *data = (int *)malloc(4 * sizeof(int));
  data[0] = 13;
  data[1] = 2;
  data[2] = 3;
  data[3] = 5;

  foo(data);

  free(data);
}
