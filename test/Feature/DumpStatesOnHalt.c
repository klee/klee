// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --stop-after-n-instructions=1 --dump-states-on-halt=true %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %T/klee-last/test000001.ktest

int main() {
  int x = 10;
  return x;
}
// CHECK: halting execution, dumping remaining states
