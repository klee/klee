// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t1.bc
// RUN: %klee --use-query-log=all:pc,all:smt2,solver:pc,solver:smt2 --write-pcs --write-cvcs --write-smt2s %t1.bc 2> %t2.log
// RUN: %kleaver -print-ast klee-last/all-queries.pc > %t3.log
// RUN: %kleaver -print-ast %t3.log > %t4.log
// RUN: diff %t3.log %t4.log
// RUN: %kleaver -print-ast klee-last/solver-queries.pc > %t3.log
// RUN: %kleaver -print-ast %t3.log > %t4.log
// RUN: diff %t3.log %t4.log
// RUN: grep "^; Query" klee-last/all-queries.smt2 | wc -l | grep -q 17
// RUN: grep "^; Query" klee-last/solver-queries.smt2 | wc -l | grep -q 10

#include <assert.h>

int constantArr[16 ] = {
  1 <<  0, 1 <<  1, 1 <<  2, 1 <<  3,
  1 <<  4, 1 <<  5, 1 <<  6, 1 <<  7,
  1 <<  8, 1 <<  9, 1 << 10, 1 << 11,
  1 << 12, 1 << 13, 1 << 14, 1 << 15
};


int main() {
  char buf[4];
  klee_make_symbolic(buf, sizeof buf);

  buf[1] = 'a';

  constantArr[klee_range(0, 16, "idx.0")] = buf[0];

  // Use this to trigger an interior update list usage.
  int y = constantArr[klee_range(0, 16, "idx.1")];  

  constantArr[klee_range(0, 16, "idx.2")] = buf[3];
  
  buf[klee_range(0, 4, "idx.3")] = 0;
  klee_assume(buf[0] == 'h');
  
  int x = *((int*) buf);
  klee_assume(x > 2);
  klee_assume(x == constantArr[12]);

  klee_assume(y != (1 << 5));

  assert(0);

  return 0;
}
