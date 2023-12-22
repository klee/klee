// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -write-sarifs --use-sym-size-alloc --use-sym-size-li --skip-not-symbolic-objects --posix-runtime --libc=uclibc -cex-cache-validity-cores --output-dir=%t.klee-out %t.bc > %t.log
// RUN: %checker %t.klee-out/report.sarif %S/pattern.sarif

#include "klee/klee.h"

#include <stdlib.h>

int main() {
  char *s;
  klee_make_symbolic(&s, sizeof(s), "s");

  *s = 100;
  s[200] = 200;
}
