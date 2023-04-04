// RUN: %clang %s -emit-llvm %O0opt -g -c  -D_FORTIFY_SOURCE=0 -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t2.bc 2> %t.log
// RUN: FileCheck %s --input-file=%t.log
// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize --output-dir=%t.klee-out %t2.bc 2> %t.log
// RUN: FileCheck %s --input-file=%t.log

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define LENGTH 5
int main(int argc, char **argv) {
  char *src = (char *)malloc(LENGTH);
  char *dst = (char *)malloc(LENGTH);

  memset(src, 42, LENGTH);
  // CHECK-NOT: calling external: memset

  memcpy(dst, src, LENGTH);
  // CHECK-NOT: calling external: memcpy

  memmove(dst, src, LENGTH);
  // CHECK-NOT: calling external: memmove

  assert(memcmp(src, dst, LENGTH) == 0);
  // CHECK-NOT: calling external: memcmp
  // CHECK-NOT: calling external: bcmp

  assert(*src == 42);
  assert(*src == *dst);
  return 0;
}
