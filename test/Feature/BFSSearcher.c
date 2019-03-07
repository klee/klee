// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-instructions=500 --search=bfs %t1.bc 2>%t2.log
// RUN: FileCheck -input-file=%t2.log %s
#include "assert.h"
#include "klee/klee.h"

int nd() {
  int r;
  klee_make_symbolic(&r, sizeof(r), "r");
  return r;
}

int main() {
  int x = 1;
  while (nd() != 0) {
    x *= 2;
  }
  // CHECK: ASSERTION FAIL
  klee_assert(0);
  return x;
}
