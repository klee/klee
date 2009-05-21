// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc
// RUN: test -f klee-last/test000001.free.err

int main() {
  int buf[4];
  free(buf); // this should give runtime error, not crash
  return 0;
}
