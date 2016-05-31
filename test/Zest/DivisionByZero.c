// RUN: %llvmgcc -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --zest %t1.bc x
// RUN: test -f %t.klee-out/test000001.div.err

#include <assert.h>
int main(int argc, char *argv[]) {
  assert(argc == 2);

  return 20 / argv[1][0];
}
