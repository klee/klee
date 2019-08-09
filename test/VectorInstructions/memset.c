// RUN: rm -rf %t.klee-out
// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: %klee --output-dir=%t.klee-out --optimize=true --exit-on-error %t1.bc

#include <string.h>

int main() {
  char arr[1024];
  memset(arr, 0, sizeof(arr));
  return arr[0];
}
