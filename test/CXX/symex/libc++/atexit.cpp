// REQUIRES: libcxx
// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 -I "%libcxx_include" -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc --libcxx %t1.bc | FileCheck %s
//
// CHECK: Returning from main
// CHECK: atexit_1
// CHECK: atexit_2

#include <iostream>
#include <cstdlib>

int main(int argc, char** args){
  auto atexithandler2 = std::atexit([](){std::cout << "atexit_2" << std::endl;});
  auto atexithandler1 = std::atexit([](){std::cout << "atexit_1" << std::endl;});
  std::cout << "Returning from main" << std::endl;
  return 0;
}
