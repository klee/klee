// REQUIRES: z3
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --solver-backend=z3 -write-paths %t.bc 2> %t.log
// RUN: cat %t.klee-out/test000001.path | wc -l | grep -q 1
// RUN: cat %t.klee-out/test000002.path | wc -l | grep -q 1
int main() {
  int a, b;
  klee_make_symbolic(&a, sizeof(int), "a");
  klee_make_symbolic(&b, sizeof(int), "b");
  klee_assume(a < 2);
  klee_assume(a >= 0);
  malloc(a);
  if (b) {
    b++; // do something
  }
  return b;
}
