// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t1.bc
// RUN: rm -rf %t1.klee-out
// RUN: %klee --output-dir=%t1.klee-out --use-alpha-equivalence=false --use-cex-cache=true --use-query-log=solver:kquery,solver:smt2 --write-kqueries --write-cvcs --write-smt2s %t1.bc 2> %t2.log
// RUN: rm -rf %t2.klee-out
// RUN: %klee --output-dir=%t2.klee-out --use-alpha-equivalence=true --use-cex-cache=true --use-query-log=solver:kquery,solver:smt2 --write-kqueries --write-cvcs --write-smt2s %t1.bc 2> %t2.log
// RUN: grep "^; Query" %t1.klee-out/solver-queries.smt2 | wc -l | grep -q 2
// RUN: grep "^; Query" %t2.klee-out/solver-queries.smt2 | wc -l | grep -q 1

#include "klee/klee.h"

int main(int argc, char **argv) {
  int a, b, c, d;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");
  klee_make_symbolic(&c, sizeof(c), "c");
  klee_make_symbolic(&d, sizeof(d), "d");
  klee_assume(a + b == 0);
  klee_assume(c + d == 0);
}
