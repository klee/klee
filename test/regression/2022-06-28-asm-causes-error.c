// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --external-calls=none --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

int main(int argc, char *argv[]) {
  // CHECK: 2022-06-28-asm-causes-error.c:[[@LINE+1]]: external calls disallowed (in particular inline asm)
  __asm__ __volatile__("movl %eax, %eax");
  return 0;
}
