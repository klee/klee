// RUN: %clang -Wno-everything %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --optimize-aggressive=false --track-coverage=branches --optimize=true --delete-dead-loops=false --use-forked-solver=false -max-memory=6008 --cex-cache-validity-cores --only-output-states-covering-new=true --dump-states-on-halt=true %t1.bc 2>&1 | FileCheck -check-prefix=CHECK %s

// RUN: rm -f %t*.gcda %t*.gcno %t*.gcov
// RUN: %cc -DGCOV %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner --coverage
// RUN: %replay %t.klee-out %t_runner
// RUN: gcov -b %t_runner-%basename_t > %t.cov.log

// RUN: FileCheck --input-file=%t.cov.log --check-prefix=CHECK-COV %s

// Branch coverage 100%, the number of branches is 1:
// CHECK-COV: Lines executed:100.00% of 11
// CHECK-COV-NEXT: Branches executed:100.00% of 2
// CHECK-COV-NEXT: Taken at least once:100.00% of 2

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
