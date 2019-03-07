// RUN: %clang %s -Wall -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t.bc
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Global should be aligned on a 128-byte boundary
int foo __attribute__((aligned(128)));

int main() {
  int bar __attribute__((aligned(256)));

  // Check alignment of global
  assert(((size_t)&foo) % 128 == 0);

  // Check alignment of local
  assert(((size_t)&bar) % 256 == 0);
  return 0;
}
