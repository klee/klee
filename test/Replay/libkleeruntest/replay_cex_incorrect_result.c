// RUN: %clang %s -S -emit-llvm -g -c -o %t.ll
// RUN: rm -rf %t.klee-out
// RUN: %klee --search=dfs --output-dir=%t.klee-out %t.ll

// This should produce four test cases.
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: test -f %t.klee-out/test000004.ktest
// RUN: test ! -f %t.klee-out/test000005.ktest

// Now try to replay with libkleeRuntest
// RUN: %cc -DPRINT_VALUE %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner

// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_1 %s
// RUN: env KTEST_FILE=%t.klee-out/test000002.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_2 %s
// RUN: env KTEST_FILE=%t.klee-out/test000003.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_3 %s
// RUN: env KTEST_FILE=%t.klee-out/test000004.ktest %t_runner 2>&1 | FileCheck -check-prefix=CHECK_4 %s

#include <klee/klee.h>
#include <stdio.h>

void f0(void) {}
void f1(void) {}
void f2(void) {}
void f3(void) {}

int main() {
  int x = klee_range(0, 256, "x");

  if (x == 17) {
    f0();
    // CHECK_4: x=17
  } else if (x == 32) {
    f1();
    // CHECK_3: x=32
  } else if (x == 99) {
    f2();
    // CHECK_2: x=99
  } else {
    klee_prefer_cex(&x, x == 0);
    f3();
    // CHECK_1: x=0
  }

#ifdef PRINT_VALUE
  printf("x=%d\n", x);
#endif

  return 0;
}
