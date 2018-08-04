// RUN: sh %S/compile_with_libcxx.sh "%llvmgxx" "%s" "%S" "%t" "%klee" "%libcxx_include"

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
