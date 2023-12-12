// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-sym-size-alloc --symbolic-allocation-threshold=0 %t1.bc 2>&1 | FileCheck %s

extern int GLOBAL[];

int main() {
  // CHECK: SymbolicSizeUnsizedGlobal.c:[[@LINE+1]]: memory error: out of bound pointer
  GLOBAL[1] = 0;
  // CHECK: SymbolicSizeUnsizedGlobal.c:[[@LINE+1]]: memory error: out of bound pointer
  GLOBAL[2] = 0;
  // CHECK-NOT: SymbolicSizeUnsizedGlobal.c:[[@LINE+1]]: memory error: out of bound pointer
  GLOBAL[0] = 0;
}
