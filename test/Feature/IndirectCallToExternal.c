// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: %klee %t1.bc

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main() {
  int (*scmp)(char*,char*) = strcmp;

  assert(scmp("hello","hi") < 0);

  return 0;
}
