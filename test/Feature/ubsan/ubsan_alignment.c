// RUN: %clang %s -fsanitize=alignment -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s %c_prefixes_8

#include "klee/klee.h"
#include <stdlib.h>

char *passthrough(__attribute__((align_value(0x8000))) char *x) {
  // CHECK-8: ubsan_alignment.c:[[@LINE+1]]: assumption of 32768 byte alignment failed
  return x;
}

void alignment_assumption_failed() {
  char *ptr = (char *)malloc(2);

  passthrough(ptr + 1);

  free(ptr);
}

void load_of_misaligned_address() {
  int x = klee_range(0, 5, "x");

  char c[] __attribute__((aligned(8))) = {0, 0, 0, 0, 1, 2, 3, 4, 5};
  int *p = (int *)&c[x];

  // CHECK: ubsan_alignment.c:[[@LINE+1]]: either misaligned address for 0x{{.*}} or invalid usage of address 0x{{.*}} with insufficient space
  volatile int result = *p;
}

int main() {
  _Bool type;

  klee_make_symbolic(&type, sizeof type, "type");

  if (type) {
    alignment_assumption_failed();
  } else {
    load_of_misaligned_address();
  }
  return 0;
}
