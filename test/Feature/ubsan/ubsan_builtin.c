// REQUIRES: arch=x86_64
//
// RUN: %clang %s -fsanitize=builtin -w -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

int main() {
  signed int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  // CHECK: builtins.cpp:[[@LINE+2]]: passing zero to either ctz() or clz(), which is not a valid argument
  __builtin_ctz(n);
  return 0;
}
