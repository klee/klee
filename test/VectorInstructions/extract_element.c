// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// constant folded away.
// RUN: %klee --output-dir=%t.klee-out --optimize=false %t1.bc > %t.stdout.log 2> %t.stderr.log
// RUN: FileCheck -check-prefix=CHECK-STDOUT -input-file=%t.stdout.log %s
// RUN: FileCheck -check-prefix=CHECK-STDERR -input-file=%t.stderr.log %s
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

typedef uint32_t v4ui __attribute__ ((vector_size (16)));
int main() {
  v4ui f = { 0, 1, 2, 3 };
  // Performing these reads should be ExtractElement instructions
  // CHECK-STDOUT: f[0]=0
  printf("f[0]=%u\n", f[0]);
  // CHECK-STDOUT-NEXT: f[1]=1
  printf("f[1]=%u\n", f[1]);
  // CHECK-STDOUT-NEXT: f[2]=2
  printf("f[2]=%u\n", f[2]);
  // CHECK-STDOUT-NEXT: f[3]=3
  printf("f[3]=%u\n", f[3]);
  // CHECK-STDERR-NOT: extract_element.c:[[@LINE+1]]: ASSERTION FAIL
  assert(f[0] == 0);
  // CHECK-STDERR-NOT: extract_element.c:[[@LINE+1]]: ASSERTION FAIL
  assert(f[1] == 1);
  // CHECK-STDERR-NOT: extract_element.c:[[@LINE+1]]: ASSERTION FAIL
  assert(f[2] == 2);
  // CHECK-STDERR-NOT: extract_element.c:[[@LINE+1]]: ASSERTION FAIL
  assert(f[3] == 3);
  // CHECK-STDERR: extract_element.c:[[@LINE+1]]: Out of bounds read when extracting element
  printf("f[4]=%u\n", f[4]); // Out of bounds
  return 0;
}
