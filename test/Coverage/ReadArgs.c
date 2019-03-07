// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: echo " --output-dir=%t.klee-out " > %t1.args
// RUN: %klee @%t1.args %t1.bc
// RUN: test -d %t.klee-out

int main() {
  return 0;
}
