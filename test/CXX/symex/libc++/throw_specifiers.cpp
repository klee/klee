// Testcase for proper handling of
// throw specifications on functions
// REQUIRES: uclibc
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -std=c++11 -c %libcxx_includes -g -nostdinc++ -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libcxx --libc=uclibc  %t.bc | FileCheck %s

#include <stdio.h>

namespace {
void throw_expected() throw(int) {
  throw 5;
}

void throw_unexpected() throw(int, double) {
  throw "unexpected string";
}
} // namespace

int main() {
  try {
    throw_expected();
  } catch (int i) {
    puts("caught expected int");
    // CHECK: caught expected int
  }

  try {
    throw_unexpected();
  } catch (char const *s) {
    puts("caught unexpected string");
    // CHECK-NOT: caught unexpected string
  }
}
