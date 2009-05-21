// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf xxx
// RUN: echo " --output-dir=xxx " > %t1.args
// RUN: %klee --read-args %t1.args %t1.bc
// RUN: test -d xxx

int main() {
  return 0;
}
