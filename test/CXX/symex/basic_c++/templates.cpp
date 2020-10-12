// REQUIRES: uclibc
// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc %t.bc 2>&1

#include <cassert>

template <typename X, typename Y>
X multiply(X a, Y b) {
  return a * b;
}

template <int A>
int add(int summand) {
  return A + summand;
}

int main(int argc, char **args) {
  assert(add<7>(3) == 10);

  assert(multiply(7, 3) == 21);

  // Check if the float arguments are handled correctly
  assert(multiply(7.1f, 3) > 21);
  assert(multiply(3, 7.1f) == 21);

  return 0;
}
