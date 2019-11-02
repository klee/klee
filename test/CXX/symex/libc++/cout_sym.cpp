// REQUIRES: libcxx
// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 -I "%libcxx_include" -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc --libcxx %t1.bc | FileCheck %s

// CHECK-DAG: greater
// CHECK-DAG: lower


#include <iostream>
#include "klee/klee.h"

int main(int argc, char** args){
  int x = klee_int("x");
  if (x > 0){
    std::cout << "greater: " << x << std::endl;
  } else {
    std::cout << "lower: " << x << std::endl;
  }
  return 0;
}
