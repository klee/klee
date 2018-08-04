// RUN: sh %S/compile_with_libcxx.sh "%llvmgxx" "%s" "%S" "%t" "%klee" "%libcxx_include"

#include <iostream>

int main(int argc, char** args){
  std::cout << "cout" << std::endl;
  std::cerr << "cerr" << std::endl;
  return 0;
}
