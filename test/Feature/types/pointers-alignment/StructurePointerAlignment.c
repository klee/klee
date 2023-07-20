// RUN: %clang %s -emit-llvm -g -c -fsanitize=alignment,null -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --ubsan-runtime --use-advanced-type-system --use-tbaa --use-lazy-initialization=all --align-symbolic-pointers=true --use-gep-opt %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdio.h>

struct Node {
  struct Node *next;
  int x;
};

void foo(struct Node *head) {
  if (head->next) {
    // CHECK-DAG: x
    printf("x\n");
    // CHECK-NOT: StructurePointerAlignment.c:[[@LINE+1]]: either misaligned address for 0x{{.*}} or invalid usage of address 0x{{.*}} with insufficient space
    head->next->x = 100;
  } else {
    // CHECK-DAG: y
    printf("y\n");
  }
}

int main() {
  struct Node head;
  klee_make_symbolic(&head, sizeof(head), "head");
  klee_prefer_cex(&head, head.x >= -10 & head.x <= 10);
  foo(&head);
  return 0;
}
