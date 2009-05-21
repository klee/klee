// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc
// RUN: test -f klee-last/test000001.free.err

int buf[4];

int main() {
  free(buf); // this should give runtime error, not crash
  return 0;
}
