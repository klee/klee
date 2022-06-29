// REQUIRES: not-z3
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --solver-backend=z3 %t1.bc 2>&1 | FileCheck %s
// CHECK: Not compiled with Z3 support
// CHECK: ERROR: Failed to create core solver

int main(int argc, char **argv) {
  return 0;
}
