// RUN: %clang  -Wno-everything %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --optimize=true --delete-dead-loops=false --use-forked-solver=false -max-memory=6008 --cex-cache-validity-cores --only-output-states-covering-new=true --dump-states-on-halt=true %t1.bc 2>&1 | FileCheck -check-prefix=CHECK %s

#include "klee-test-comp.c"

extern int __VERIFIER_nondet_int(void);

int main() {
  int *x = alloca(sizeof(int));
  int *y = alloca(sizeof(int));
  int *z = alloca(sizeof(int));
  *x = __VERIFIER_nondet_int();
  *y = __VERIFIER_nondet_int();
  *z = __VERIFIER_nondet_int();

  while (*x > 0) {
    *x = *x + *y;
    *y = *z;
    *z = -(*z) - 1;
  }
}

// CHECK: generated tests = 3
