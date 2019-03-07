// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t.bc > %t.log

#include <stdlib.h>

int main(int argc, char *argv[]) {
  int *a = (int *)memalign(8, sizeof(int) * 5);
  for (int i = 0; i < 5; ++i) {
    a[i] = (i * 100) % 23;
  }
  free(a);
  return 0;
}
