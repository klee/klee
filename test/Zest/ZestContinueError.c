// RUN: %llvmgcc -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --posix-runtime --zest --zest-continue-after-error %t1.bc ab
// RUN: test -f %t.klee-out/test000002.ptr.err

#include <stdio.h>
int main(int argc, char **argv) {
  int v['b'];

  if (argv[1][0] > 0) {
    v[argv[1][0]] = 10;
    v[argv[1][1] - 1] = 32;
  }
  printf("v[\'a\']=%d\n", v['a']);
  return 0;
}
