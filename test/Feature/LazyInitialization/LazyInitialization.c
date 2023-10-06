// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --skip-not-symbolic-objects --use-timestamps=false --use-guided-search=none %t.bc > %t.log
// RUN: FileCheck %s -input-file=%t.log

struct Node {
  int x;
  struct Node *next;
};

int len_bound(struct Node *head, int bound) {
  int len = 0;
  while (head != 0 && bound > 0) {
    ++len;
    --bound;
    head = head->next;
  }
  return head != 0 ? -1 : len;
}

#define SIZE 2

int main() {
  struct Node *node;
  klee_make_symbolic(&node, sizeof(node), "node");
  int result = len_bound(node, SIZE);

  // CHECK-DAG: Yes
  // CHECK-DAG: No
  if (node->x > 0) {
    printf("Yes\n");
  } else {
    printf("No\n");
  }
  return 0;
}
