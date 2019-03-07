// REQUIRES: z3
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -solver-backend=z3 -write-cvcs -write-smt2s -debug-z3-dump-queries=%t.smt2   %t1.bc
// RUN: cat %t.klee-out/test000001.smt2 | FileCheck --check-prefix=TEST-CASE %s
// RUN: cat %t.klee-out/test000002.smt2 | FileCheck --check-prefix=TEST-CASE %s
// RUN: cat %t.smt2 | FileCheck %s

#include "klee/klee.h"

int main(int argc, char **argv) {
  // CHECK-DAG: (assert (= (select const_arr11 #x00000000) #x67))
  // CHECK-DAG: (assert (= (select const_arr11 #x00000001) #x79))
  // CHECK-DAG: (assert (= (select const_arr11 #x00000002) #x7a))
  // CHECK-DAG: (assert (= (select const_arr11 #x00000003) #x00))
  // TEST-CASE-DAG: (assert (=  (select const_arr1 (_ bv0 32) ) (_ bv103 8) ) )
  // TEST-CASE-DAG: (assert (=  (select const_arr1 (_ bv1 32) ) (_ bv121 8) ) )
  // TEST-CASE-DAG: (assert (=  (select const_arr1 (_ bv2 32) ) (_ bv122 8) ) )
  // TEST-CASE-DAG: (assert (=  (select const_arr1 (_ bv3 32) ) (_ bv0 8) ) )
  char c[4] = {'g', 'y', 'z', '\0'};
  unsigned i;
  klee_make_symbolic(&i, sizeof i, "i");
  klee_assume(i < 4);
  if (c[i] < 'y')
    return 0;
  else
    return 1;
}
