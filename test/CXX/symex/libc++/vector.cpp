// REQUIRES: libcxx
// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 -I "%libcxx_include" -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc --libcxx %t1.bc 2>&1 | FileCheck %s
//
// CHECK: KLEE: done: completed paths = 1

#include "klee/klee.h"
#include <vector>

int main(int argc, char **args) {
  std::vector<int> a;
  a.push_back(klee_int("Test"));
  return a.at(0);
}
