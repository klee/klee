// RUN: %llvmgcc %s -emit-llvm -m32 -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc

// This simply tests that allocation works when
// running 32-bit code

// FIXME: This test only needs to run on x86_64
int main() {
  int a[100000];
  for (int i=0; i < 5000; ++i) {
    a[i] = i;
  }
  return 0;
}
