/* The scalarizer pass in LLVM 11 was changed to generate, for a
 write of the form f[k] = v, with f a 4-element vector:
 if k == 0 => f[0] = v
 if k == 1 => f[1] = v
 if k == 2 => f[2] = v
 if k == 3 => f[3] = v

 Therefore, even though an OOB write access might exist at the source
 code level (e.g., f[5] = v), no such OOB accesses exist anymore at
 the LLVM IR level.

 So unlike in the LLVM < 11 test, here we test that the contents of
 the vector is unmodified after the OOB write.
*/

// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// constant folded away.
// RUN: %klee --output-dir=%t.klee-out --optimize=false --exit-on-error %t1.bc

#include "klee/klee.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t v4ui __attribute__((vector_size(16)));
int main() {
  v4ui f = {1, 2, 3, 4};
  int k = klee_range(0, 10, "k");

  if (k < 4) {
    f[5] = 3; // Concrete out-of-bounds write
    assert(f[0] == 1);
    assert(f[1] == 2);
    assert(f[2] == 3);
    assert(f[3] == 4);
  }
  else {
    f[k] = 255; // Symbolic out-of-bounds write
    assert(f[0] == 1);
    assert(f[1] == 2);
    assert(f[2] == 3);
    assert(f[3] == 4); 
  }

  return 0;
}
