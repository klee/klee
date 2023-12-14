// RUN: %clang -DKLEE_EXECUTION %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc -sym-arg 1

// RUN: %cc %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -O0 -o %t2
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %klee-replay %t2 %t.klee-out/test000001.ktest | FileCheck --check-prefix=REPLAY %s
// REPLAY: Yes

#include <stdio.h>
#include "klee/klee.h"

int main (int argc, char** argv) {
  if (argc < 2) {
    return 1;
  }
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  //If Replay works properly printf will be executed
  printf("Yes\n");
}