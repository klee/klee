// RUN: %llvmgxx %s -emit-llvm -g -c -o %t1.bc
// RUN: %klee %t1.bc
// RUN: test -f klee-last/test000001.external.err

extern "C" void poof(void);

int main() {
  poof();

  return 0;
}
