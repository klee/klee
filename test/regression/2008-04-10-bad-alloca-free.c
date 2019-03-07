// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

void f(int *addr) {
  klee_make_symbolic(addr, sizeof *addr, "moo");
}

int main() {
  int x;
  f(&x);
  return x;
}
