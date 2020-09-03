// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge  %t.bc 2>&1 | FileCheck %s

// CHECK: generated tests = 2{{$}}

// Now try to replay with libkleeRuntest
// RUN: %cc %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner > %t.txt
// RUN: env KTEST_FILE=%t.klee-out/test000002.ktest %t_runner >> %t.txt
// RUN: FileCheck --check-prefix=REPLAY --input-file %t.txt %s

// REPLAY-DAG: 0
// REPLAY-DAG: 1


#include "klee/klee.h"

#include <stdio.h>

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

  if (foo > 0)
    foo = 1;
  printf("foo = %d\n", foo);
  
  return 0;
}
