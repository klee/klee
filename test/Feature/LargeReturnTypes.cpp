// REQUIRES: not-msan
// Requires instrumented C++ library for msan
// REQUIRES: not-darwin
// REQUIRES: not-freebsd
// RUN: %clangxx -g -fno-exceptions -emit-llvm %O0opt -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --write-no-tests --exit-on-error %t.bc > %t.log

/* Tests the ability to call external functions which return large values
 * (i.e. structs).  In this test case, fstream::ftellg() returns a
 * streampos (an {i64, i64} pair) which is implicitly converted to a size_t. */

// This test currently doesn't work on darwin because this isn't how things work
// in libc++. This test should be rewritten to not depend on an external
// dependency.

#include <fstream>

using namespace std;

size_t fileSize(const char *filename) {
  fstream f(filename, fstream::in | fstream::binary);

  if (f.is_open()) {
    f.seekg(0, fstream::end);
    size_t fileSize = f.tellg();
    return fileSize;
  }

  return (size_t) -1;
}

int main(void) {
  size_t size = fileSize("/bin/sh");
  return (size != (size_t) -1) ? 0 : 1;
}
