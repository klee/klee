// REQUIRES: geq-llvm-10.0
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64 > %t.log
// RUN: FileCheck %s -input-file=%t.log

#include <stdio.h>

int main() {
  char a[8];
  gets(a);
  if (a[0] == 'u' && a[1] == 't' && a[2] == 'b' && a[3] == 'o' && a[4] == 't') {
    printf("%c%c%c%c%c\n", a[0], a[1], a[2], a[3], a[4]);
    // CHECK: utbot
    return 1;
  }
  return 0;
}
