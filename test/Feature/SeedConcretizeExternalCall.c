// RUN: %clang -emit-llvm -c -g %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=TestGen %t.bc "initial"
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: not test -f %t.klee-out/test000002.ktest

// RUN: rm -rf %t.klee-out-2
// RUN: %klee --external-calls=all --output-dir=%t.klee-out-2 --seed-file %t.klee-out/test000001.ktest %t.bc
// RUN: grep -q "ASSERTION FAIL" %t.klee-out-2/messages.txt

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void TestGen() {
  unsigned z;
  klee_make_symbolic(&z, sizeof z, "z");
  klee_assume(z == 0xBADA55);
}

int main(int argc, char **argv) {
  int z;
  klee_make_symbolic(&z, sizeof z, "z");
  assert(abs(z) != 0xBADA55);
  return 0;
}
