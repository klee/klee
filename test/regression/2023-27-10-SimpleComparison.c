// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --debug-assignment-validating-solver=false --use-fast-cex-solver=false --use-cex-cache=false --use-branch-cache=false --use-alpha-equivalence=true --use-independent-solver=false --use-concretizing-solver=false --output-dir=%t.klee-out --skip-not-symbolic-objects --use-timestamps=false --use-guided-search=none %t.bc

#include <assert.h>

struct Node {
  int *x;
};

int main() {
  struct Node *nodeA;
  struct Node *nodeB;
  klee_make_symbolic(&nodeA, sizeof(nodeA), "nodeA");
  klee_make_symbolic(&nodeB, sizeof(nodeB), "nodeB");

  if (nodeA && nodeB && nodeA == nodeB && (*nodeA->x * 2) != (*nodeA->x + *nodeB->x)) {
    assert(0);
  }
  return 0;
}
