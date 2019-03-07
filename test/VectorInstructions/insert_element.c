// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// constant folded away.
// RUN: %klee --output-dir=%t.klee-out --optimize=false  %t1.bc > %t.stdout.log 2> %t.stderr.log
// RUN: FileCheck -check-prefix=CHECK-STDOUT -input-file=%t.stdout.log %s
// RUN: FileCheck -check-prefix=CHECK-STDERR -input-file=%t.stderr.log %s
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

typedef uint32_t v4ui __attribute__ ((vector_size (16)));
int main() {
  v4ui f = { 0, 1, 2, 3 };
  klee_print_expr("f:=", f);
  // Performing these writes should be InsertElement instructions
  f[0] = 255;
  klee_print_expr("f after write to [0]", f);
  f[1] = 128;
  klee_print_expr("f after write to [1]", f);
  f[2] = 1;
  klee_print_expr("f after write to [2]", f);
  f[3] = 19;
  klee_print_expr("f after write to [3]", f);
  // CHECK-STDOUT: f[0]=255
  printf("f[0]=%u\n", f[0]);
  // CHECK-STDOUT: f[1]=128
  printf("f[1]=%u\n", f[1]);
  // CHECK-STDOUT: f[2]=1
  printf("f[2]=%u\n", f[2]);
  // CHECK-STDOUT: f[3]=19
  printf("f[3]=%u\n", f[3]);
  // CHECK-STDERR-NOT: insert_element.c:[[@LINE+1]]: ASSERTION FAIL
  assert(f[0] == 255);
  // CHECK-STDERR-NOT: insert_element.c:[[@LINE+1]]: ASSERTION FAIL
  assert(f[1] == 128);
  // CHECK-STDERR-NOT: insert_element.c:[[@LINE+1]]: ASSERTION FAIL
  assert(f[2] == 1);
  // CHECK-STDERR-NOT: insert_element.c:[[@LINE+1]]: ASSERTION FAIL
  assert(f[3] == 19);

  // CHECK-STDERR: insert_element.c:[[@LINE+1]]: Out of bounds write when inserting element
  f[4] = 255; // Out of bounds write
  return 0;
}
