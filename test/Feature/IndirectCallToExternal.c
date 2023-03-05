// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  int (*scmp)(char *, char *) = strcmp;

  assert(scmp("hello", "hi") < 0);

  return 0;
}
