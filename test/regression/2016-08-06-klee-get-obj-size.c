// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc
// RUN: test -f %t.klee-out/test000001.assert.err

#include "klee/klee.h"

#include <assert.h>

int main() {
  char s[5];
  assert(5 != klee_get_obj_size(s));
  return 0;
}
