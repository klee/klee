// REQUIRES: lt-llvm-11.0
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// constant folded away.
// RUN: %klee --output-dir=%t.klee-out --optimize=false  %t1.bc > %t.stdout.log 2> %t.stderr.log
// RUN: FileCheck -input-file=%t.stderr.log %s

#include "klee/klee.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t v4ui __attribute__((vector_size(16)));
int main() {
  v4ui f = {1, 2, 3, 4};
  int k = klee_range(0, 10, "k");

  if (k == 0) {
    // CHECK-DAG: [[@LINE+1]]: Out of bounds write when inserting element
    f[4] = 255; // Out of bounds write
  }

  if (k == 1) {
    // CHECK-DAG: [[@LINE+1]]: Out of bounds read when extracting element
    printf("f[4] = %u\n", f[5]); // Out of bounds
  }

  if (k > 6) {
    // Performing read should be ExtractElement instruction.
    // For now this is an expected limitation.
    // CHECK-DAG: [[@LINE+1]]: ExtractElement, support for symbolic index not implemented
    uint32_t readValue = f[k];
  }
  else {
    // Performing write should be InsertElement instructions.
    // For now this is an expected limitation.
    // CHECK-DAG: [[@LINE+1]]: InsertElement, support for symbolic index not implemented
    f[k] = 255;
  }

  return 0;
}
