// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -exit-on-error %t.bc

#include <assert.h>
#include <klee/klee.h>

int f1(int a, int b) {
  return a + b;
}

int f2(int a, int b) {
  int i;
  for (i = 0; i < sizeof(b) * 8; i++)
    a += (((b >> i) & 1) << i);

  return a;
}

int main(int argc, char **argv) {
  int a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");

  klee_assert(f1(a, b) == f2(a, b));

  return 0;
}

