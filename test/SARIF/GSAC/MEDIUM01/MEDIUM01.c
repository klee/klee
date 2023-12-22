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
 * Based on CVE-2022-0185
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE (1ULL << PAGE_SHIFT)

int foo(unsigned int size, const char *key) {
  size_t len = 3;

  if (len > PAGE_SIZE - 2 - size) {
    printf("Too large\n");
    return -1;
  }

  // 'strlen' can't find '\0'
  len = strlen(key); // buffer-overflow
  return len;
}

int main() {
  unsigned int size = 4294967295;
  char *key = malloc(10);
  // After 'memcpy()' 'key' doesn't have a '\0' symbol
  memcpy(key, "asddds4323", 10);

  foo(size, key);

  free(key);
}
