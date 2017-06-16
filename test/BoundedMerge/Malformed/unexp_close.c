// RUN: %S/setup_test.sh "%llvmgcc" %s %t "-max-time=2" | FileCheck %s
// CHECK: ran into a close at
// CHECK: generated tests = 2

#include <klee/klee.h>

int main(int argc, char **args) {

  int x;
  int a;
  int foo = 0;

  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&a, sizeof(a), "a");

  if (a == 0) {
    klee_open_merge();

    if (x == 1) {
      foo = 5;
    } else if (x == 2) {
      foo = 6;
    } else {
      foo = 7;
    }

    klee_close_merge();
  }
  klee_close_merge();

  return foo;
}
