// RUN: sh %S/compile_with_libcxx.sh "%llvmgxx" "%s" "%S" "%t" "%klee" "%libcxx_include" | FileCheck %s

// CHECK: KLEE: done: completed paths = 1

#include "klee/klee.h"
#include <vector>

int main(int argc, char **args) {
  std::vector<int> a;
  a.push_back(klee_int("Test"));
  return a.at(0);
}
