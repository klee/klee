// REQUIRES: not-msan
// Disabling msan because it times out on CI
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -c -std=c++11 %libcxx_includes -g -nostdinc++ -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --libcxx %t1.bc 2>&1 | FileCheck %s

#include "klee/klee.h"

#include <cstdio>
#include <stdexcept>

int main(int argc, char **args) {
  try {
    throw std::runtime_error("foo");
  } catch (const std::runtime_error &ex) {
    std::puts(ex.what());
    // CHECK: foo
  }
}

// CHECK: KLEE: done: completed paths = 1
