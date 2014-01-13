// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %T/klee-last/test000001.ptr.err -o -f %T/klee-last/test000002.ptr.err
// RUN: not test -f %T/klee-last/test000001.ptr.err -a -f %T/klee-last/test000002.ptr.err
// RUN: not test -f %T/klee-last/test000003.ktest

unsigned klee_urange(unsigned start, unsigned end) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x);
  if (x-start>=end-start) klee_silent_exit(0);
  return x;
}

int main() {
  int *x = malloc(sizeof(int));
  // FIXME: Use newer FileCheck syntax to support relative line numbers
  // CHECK: InAndOutOfBounds.c:18: memory error: out of bound pointer
  x[klee_urange(0,2)] = 1;
  free(x);
  return 0;
}
