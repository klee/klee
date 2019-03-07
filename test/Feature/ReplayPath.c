// RUN: %clang %s -emit-llvm %O0opt -DCOND_EXIT -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-paths %t1.bc > %t3.good

// RUN: %clang %s -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --replay-path %t.klee-out/test000001.path %t2.bc > %t3.log
// RUN: diff %t3.log %t3.good

#include <unistd.h>
#include <stdio.h>

void cond_exit() {
#ifdef COND_EXIT
  klee_silent_exit(0);
#endif
}

int main() {
  int res = 1;
  int x;

  klee_make_symbolic(&x, sizeof x, "x");

  if (x&1) res *= 2; else cond_exit();
  if (x&2) res *= 3; else cond_exit();
  if (x&4) res *= 5; else cond_exit();

  // get forced branch coverage
  if (x&2) res *= 7;
  if (!(x&2)) res *= 11;
  printf("res: %d\n", res);
 
  return 0;
}
