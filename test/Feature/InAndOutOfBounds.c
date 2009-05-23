// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc
// RUN: test -f klee-last/test000001.ptr.err -o -f klee-last/test000002.ptr.err 
// RUN: test ! -f klee-last/test000001.ptr.err -o ! -f klee-last/test000002.ptr.err 
// RUN: test ! -f klee-last/test000003.ktest

unsigned klee_urange(unsigned start, unsigned end) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x);
  if (x-start>=end-start) klee_silent_exit(0);
  return x;
}

int main() {
  int *x = malloc(4);
  x[klee_urange(0,2)] = 1;
  free(x);
  return 0;
}
