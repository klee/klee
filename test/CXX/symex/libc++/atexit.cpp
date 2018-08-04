// RUN: sh %S/compile_with_libcxx.sh "%llvmgxx" "%s" "%S" "%t" "%klee" "%libcxx_include" | FileCheck %s
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
