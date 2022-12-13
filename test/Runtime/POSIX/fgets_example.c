// REQUIRES: geq-llvm-10.0
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: test -f %t.klee-out/test000004.ktest
// RUN: test -f %t.klee-out/test000005.ktest
// RUN: test -f %t.klee-out/test000006.ktest
// RUN: test -f %t.klee-out/test000007.ktest
// RUN: test -f %t.klee-out/test000008.ktest
// RUN: test -f %t.klee-out/test000009.ktest
// RUN: test -f %t.klee-out/test000010.ktest
// RUN: test -f %t.klee-out/test000011.ktest
// RUN: test -f %t.klee-out/test000012.ktest
// RUN: test -f %t.klee-out/test000013.ktest
// RUN: test -f %t.klee-out/test000014.ktest
// RUN: test -f %t.klee-out/test000015.ktest
// RUN: test -f %t.klee-out/test000016.ktest
// RUN: test -f %t.klee-out/test000017.ktest
// RUN: test -f %t.klee-out/test000018.ktest
// RUN: test -f %t.klee-out/test000019.ktest
// RUN: test -f %t.klee-out/test000020.ktest
// RUN: test -f %t.klee-out/test000021.ktest
// RUN: not test -f %t.klee-out/test000022.ktest

#include <stdio.h>

int main() {
  char a[8];
  fgets(a, 6, stdin);
  if (a[0] == 'u' && a[1] == 't' && a[2] == 'b' && a[3] == 'o' && a[4] == 't') {
    return 1;
  }
  return 0;
}
