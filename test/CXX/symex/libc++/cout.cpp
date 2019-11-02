// REQUIRES: libcxx
// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 -I "%libcxx_include" -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc --libcxx %t1.bc 2>&1 | FileCheck %s

// CHECK-DAG: cout
// CHECK-DAG: cerr

#include <iostream>

int main(int argc, char** args){
  std::cout << "cout" << std::endl;
  std::cerr << "cerr" << std::endl;
  return 0;
}
