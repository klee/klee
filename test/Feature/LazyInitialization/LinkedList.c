// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --skip-not-symbolic-objects %t.bc > %t.log
// RUN: FileCheck %s -input-file=%t.log
#include "klee/klee.h"

struct Node {
  int x;
  struct Node *next;
};

#define SIZE 1

int main() {
  struct Node *a;
  klee_make_symbolic(&a, sizeof(a), "a");

  // CHECK-DAG: Yes
  // CHECK-DAG: No
  for (unsigned i = 0; i < SIZE && a != 0; ++i) {
    if (a->x > 0) {
      printf("Yes\n");
      return 0;
    }
    if (a->x < 0) {
      printf("No\n");
      return 0;
    }
    a = a->next;
  }

  return 0;
}
