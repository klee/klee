// RUN: %llvmgxx %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.external.err

extern "C" void poof(void);

int main() {
  // CHECK: failed external call: poof
  poof();

  return 0;
}
