// RUN: %llvmgcc -emit-llvm -c -o %t1.bc %s
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --use-concrete-path %t1.bc Aa
// RUN: test -f %t.klee-out/test000001.ptr.err

#include <stdio.h>
#include <assert.h>

int main(int argc, char **argv) {
  int a[100];

  assert(argc == 2);
  if (argv[1][1] == 'a')
    a[argv[1][0]] = 4;

  return 0;
}
