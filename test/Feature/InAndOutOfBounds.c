// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.ptr.err -o -f %t.klee-out/test000002.ptr.err
// RUN: not test -f %t.klee-out/test000001.ptr.err -a -f %t.klee-out/test000002.ptr.err
// RUN: not test -f %t.klee-out/test000003.ktest

unsigned klee_urange(unsigned start, unsigned end) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x, "x");
  if (x - start >= end - start)
    klee_silent_exit(0);
  return x;
}

int main() {
  int *x = malloc(sizeof(int));
  // CHECK: InAndOutOfBounds.c:[[@LINE+1]]: memory error: out of bound pointer
  x[klee_urange(0, 2)] = 1;
  free(x);
  return 0;
}
