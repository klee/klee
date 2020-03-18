// RUN: rm -rf %t.klee-out
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: %klee --output-dir=%t.klee-out --optimize=true --exit-on-error %t1.bc

#include <string.h>

int main(int argc, char** argv) {
  char arr[1000];
  memset(arr, 1, sizeof(arr));
  return arr[argc];
}
