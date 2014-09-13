// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc
// RUN: test -f %t.klee-out/test000001.ktest

int main() {
  unsigned x, y[4];

  klee_make_symbolic(&x, sizeof x, "x");
  if (x>=4) klee_silent_exit(0);
  
  y[x] = 0;

  if (x) { // force branch so y is copied
    klee_make_symbolic(&y, sizeof y, "y");
    if (y[x]==0) klee_silent_exit(0);
    return 0; // should be reachable
  } else {
    // force read here in case we try to optimize copies smartly later
    if (y[x]==0) klee_silent_exit(0);
    return 0; // not reachable
  }
}
