// REQUIRES: not-darwin
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --posix-runtime --skip-not-lazy-initialized --skip-not-symbolic-objects %t.bc > %t.log

// RUN: %cc %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner

// RUN: rm -f %t_runner.log
// RUN: %replay %t.klee-out %t_runner > %t_runner.log
// RUN: FileCheck -input-file=%t_runner.log %s

#include <stdlib.h>

struct Data {
  int x;
  int y;
};

struct Node {
  int *x;
  struct Data *data;
  struct Node *next;
};

int hard_list_and_pointers(struct Node *node) {
  if (node != NULL &&
      node->next != NULL &&
      node->x != NULL &&
      node->next->data != NULL &&
      node->next->data == node->data &&
      &(node->next->data->y) == node->x &&
      node->next->data->y == *node->x) {
    if (klee_is_replay()) { // only print x on replay runs
      printf("result = 2\n");
      // CHECK: result = 2
    }
    return 2;
  }
  return 3;
}

int main() {
  struct Node node;
  klee_make_symbolic(&node, sizeof(node), "node");
  int result;
  klee_make_symbolic(&result, sizeof(result), "result");
  int tmp = hard_list_and_pointers(&node);
  klee_assume(tmp == result);
  return 0;
}
