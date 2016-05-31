// RUN: %llvmgcc -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --posix-runtime --zest %t1.bc a
// RUN: test -f %t.klee-out/test000002.assert.err
// XFAIL: *

#include <assert.h>

int main(int argc, char **argv) {
  assert(argv[1][0] != 'd' && argv[1][1] != 'e' && "XXX");
  return 0;
}
