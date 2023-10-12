// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --debug-assignment-validating-solver=false --use-lazy-initialization=only --use-fast-cex-solver=false --use-cex-cache=false --use-branch-cache=false --use-alpha-equivalence=true --use-independent-solver=false --use-concretizing-solver=false --output-dir=%t.klee-out --skip-not-symbolic-objects --use-timestamps=false --use-guided-search=none %t.bc 2>&1 | FileCheck -check-prefix=CHECK %s

#include "klee/klee.h"
#include <assert.h>

struct Node {
  int *x;
};

int main() {
  struct Node *nodeA;
  struct Node *nodeB;
  klee_make_symbolic(&nodeA, sizeof(struct Node *), "nodeA");
  klee_make_symbolic(&nodeB, sizeof(struct Node *), "nodeB");

  if (nodeA && nodeB && nodeA == nodeB && (*nodeA->x * 2) != (*nodeA->x + *nodeB->x)) {
    // CHECK-NOT: 2023-27-10-SimpleComparison.c:[[@LINE+1]]: ASSERTION FAIL
    assert(0);
  }
  return 0;
}
