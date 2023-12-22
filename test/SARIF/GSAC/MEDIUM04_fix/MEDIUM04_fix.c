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
 * Based on CVE-2023-38559
 */
#include <stdlib.h>

void devn_pcx_write_rle(const char *from, const char *end, int step) {
  while (from < end) {
    char data = *from;

    from += step;
    if (from >= end || data != *from) {
      return;
    }
    from += step;
  }
}

int main() {
  char *a = malloc(sizeof(char));
  a[0] = 'a';
  devn_pcx_write_rle(a, a + 4, 4);
  free(a);
}
