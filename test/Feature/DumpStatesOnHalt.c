// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --stop-after-n-instructions=1 --dump-states-on-halt=true %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %T/klee-last/test000001.ktest

int main(int argc, char** argv) {
  int x = 1;
  if (argc == 1)
    x = 0;
  return x;
}
// CHECK: halting execution, dumping remaining states
