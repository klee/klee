// RUN: %clang -emit-llvm -c -g %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=TestGen %t.bc "initial"
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: not test -f %t.klee-out/test000002.ktest

// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --seed-file %t.klee-out/test000001.ktest %t.bc
// RUN: grep -q "memory error: out of bound pointer" %t.klee-out-2/messages.txt

#include <stdio.h>
#include <stdlib.h>

void TestGen() {
  unsigned z;
  klee_make_symbolic(&z, sizeof z, "z");
  klee_assume((z & 0xFF) == (0xBADC0DED & 0xFF));
}

int main(int argc, char **argv) {
  unsigned z;
  klee_make_symbolic(&z, sizeof z, "z");
  unsigned char *p = (unsigned char *) malloc((z & 0xFF) + 1);
  if (z == 0xBADC0DED) free(p);
  p[0] = z;
  return 0;
}
