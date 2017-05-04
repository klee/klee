// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=nurs:covnew --use-batching-search" | FileCheck %s
// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=bfs" | FileCheck %s
// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=dfs" | FileCheck %s
// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=random-path" | FileCheck %s
// RUN: %S/setup_test.sh "%llvmgcc" %s %t "--search=nurs:covnew" | FileCheck %s

// CHECK: open merge:
// There will be 20 'close merge' statements. Only checking a few, the generated
// test count will confirm that the merge was closed correctly
// CHECK: close merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: generated tests = 2

#include <klee/klee.h>

int main(int argc, char** args){

  int x;

  klee_make_symbolic(&x, sizeof(x), "x");
  x = x % 20;

  klee_open_merge();
  for (int i = 0; i < x; ++i){
    if (x % 3 == 0){
      klee_close_merge();
      return 1;
    }
  }
  klee_close_merge();

  return 0;
}
