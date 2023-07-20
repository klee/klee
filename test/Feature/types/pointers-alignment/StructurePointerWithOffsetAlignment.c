// RUN: %clang %s -emit-llvm -g -c -fsanitize=alignment,null -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --ubsan-runtime --use-advanced-type-system --use-tbaa --use-lazy-initialization=all --align-symbolic-pointers=true --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>

struct Node {
  struct Node *next;
  int x;
};

void foo(struct Node *head, int n) {
  char *pointer = head->next;
  pointer += n;
  head = (struct Node *)pointer;

  if ((n & (sizeof(int) - 1)) == 0) {
    // CHECK-DAG: x
    printf("x\n");
    // CHECK-DAG: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: null-pointer-use
    head->x = 100;
  } else {
    // CHECK-DAG: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: misaligned-pointer-use
    head->x = 100;
  }
}

int main() {
  struct Node head;
  klee_make_symbolic(&head, sizeof(head), "head");
  int n = klee_range(0, 4, "n");
  foo(&head, n);
  return 0;
}
