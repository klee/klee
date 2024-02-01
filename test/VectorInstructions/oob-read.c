// XFAIL: *

/* The scalarizer pass in LLVM 11 was changed to generate, for a
 read f[k], with k symbolic and f a 4-element vector:
 if k == 0 => f[0]
 elif k == 1 => f[1]
 elif k == 2 => f[2]
 elif k == 3 => f[3]
 else ==> undef

 Therefore, even though an OOB access might exist at the source code
 level, no such OOB accesses exist anymore at the LLVM IR level.

 And since undef is currently treated in KLEE as 0, an overflowing
 access is always translated as f[0], which may lead to future
 problems being missed.

 This test is marked as XFAIL as a reminder that we need to fix this
 behaviour, most likely by having undef return a new symbolic variable.
*/

// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// constant folded away.
// RUN: %klee --output-dir=%t.klee-out --optimize=false --exit-on-error %t1.bc 2>%t.log
// RUN: FileCheck -input-file=%t.stderr.log %s

#include "klee/klee.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t v4ui __attribute__((vector_size(16)));
int main() {
  v4ui f = {1, 2, 3, 4};
  int k = klee_range(4, 10, "k");

  uint32_t v = f[k]; // Symbolic out-of-bounds read
  v = f[v];          // This should trigger an error, but currently this returns f[0] = 1
  assert(v != 1);

  return 0;
}
