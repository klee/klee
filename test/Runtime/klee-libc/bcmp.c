// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=klee %t1.bc

// test bcmp for sizes including zero

#include "klee/klee.h"

#include <assert.h>
#include <stdlib.h>
#include <strings.h>

int main() {
  for (int i = 0; i < 5; ++i) {
    void *s = malloc(i);
    if (s) {
      klee_make_symbolic(s, i, "s");
      assert(0 == bcmp(s, s, i));
      free(s);
    }
  }
  return 0;
}
