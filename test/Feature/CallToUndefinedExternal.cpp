// RUN: %llvmgxx %s -emit-llvm -g -c -o %t1.bc
// RUN: %klee %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %T/klee-last/test000001.external.err

extern "C" void poof(void);

int main() {
  // CHECK: failed external call: poof
  poof();

  return 0;
}
