// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=nurs:covnew --use-batching-search" | FileCheck %s
// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=bfs" | FileCheck %s
// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=dfs" | FileCheck %s
// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=random-path" | FileCheck %s
// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=nurs:covnew" | FileCheck %s

// CHECK: open merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: generated tests = 2
#include <klee/klee.h>

int main(int argc, char** args){

  int x;
  int a;
  int foo = 0;

  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&a, sizeof(a), "a");

  if (a == 0){
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

  return foo;
}
