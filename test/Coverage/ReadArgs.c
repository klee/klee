// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %T/xxx
// RUN: echo " --output-dir=%T/xxx " > %t1.args
// RUN: %klee --read-args %t1.args %t1.bc
// RUN: test -d %T/xxx

int main() {
  return 0;
}
