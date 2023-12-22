// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -write-sarifs --use-sym-size-alloc --use-sym-size-li --skip-not-symbolic-objects --posix-runtime --libc=uclibc -cex-cache-validity-cores --output-dir=%t.klee-out %t.bc > %t.log
// RUN: %checker %t.klee-out/report.sarif %S/pattern.sarif

#include "klee/klee.h"

#include <stdlib.h>

int main() {
  int n;
  klee_make_symbolic(&n, sizeof(n), "n");

  char *s = (char *)malloc(n);
  s[1] = 10;
  s[2] = 20;
  s[0] = 0;
}
