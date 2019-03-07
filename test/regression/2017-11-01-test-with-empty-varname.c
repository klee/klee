// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc
// RUN: FileCheck %s --input-file=%t.klee-out/warnings.txt

int main() {
  unsigned a;

  klee_make_symbolic(&a, sizeof(a), "");
// CHECK-NOT: KLEE: WARNING: unable to write output test case, losing it
}
