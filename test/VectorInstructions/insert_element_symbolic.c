// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t.log 2>&1
// RUN: FileCheck -input-file=%t.log %s
#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

typedef uint32_t v4ui __attribute__ ((vector_size (16)));
int main() {
  v4ui f = { 0, 1, 2, 3 };
  unsigned index = 0;
  klee_make_symbolic(&index, sizeof(unsigned), "index");
  // Performing write should be InsertElement instructions.
  // For now this is an expected limitation.
  // CHECK: insert_element_symbolic.c:[[@LINE+1]]: InsertElement, support for symbolic index not implemented
  f[index] = 255;
  klee_print_expr("f after write to [0]", f);
  return 0;
}
