// RUN: %clang %s -m32 -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error > %t2.out 2>&1 || true
// RUN: FileCheck %s -input-file=%t2.out

// CHECK: KLEE: WARNING: Module and host target triples do not match
// CHECK-NEXT: This may cause unexpected crashes or assertion violations.
int main(void) {}
