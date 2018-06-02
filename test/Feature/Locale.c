// RUN: %llvmgcc -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc > %t.log

#include "klee/klee.h"

int main(int argc, char **args) {
  union locale_bytes {
    locale_t l;
    char *b;
  } x;
  x.l = newlocale(LC_ALL_MASK, "C", 0);
  x.b[0] = 0xC2;
  if (klee_int("x") < 10) {
    x.b[1] = ~x.b[0];
  }
  for (int i = 0; i < klee_int("y") % 3; ++i) {
    x.b[i + 2] = x.b[i + 1] ^ x.b[i];
  }
  freelocale(x.l);
  return 0;
}
