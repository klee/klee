// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --exit-on-error %t1.bc

void f(int *addr) {
  klee_make_symbolic(addr, sizeof *addr, "moo");
}

int main() {
  int x;
  f(&x);
  return x;
}
