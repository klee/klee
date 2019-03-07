// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t-bfs.klee-out
// RUN: rm -rf %t-dfs.klee-out
// RUN: rm -rf %t-bfs-dfs.klee-out
// RUN: rm -rf %t-dfs-bfs.klee-out
// RUN: %klee -output-dir=%t-bfs.klee-out -search=bfs %t.bc >%t-bfs.out
// RUN: %klee -output-dir=%t-dfs.klee-out -search=dfs %t.bc >%t-dfs.out
// RUN: %klee -output-dir=%t-bfs-dfs.klee-out -search=bfs -search=dfs %t.bc >%t-bfs-dfs.out
// RUN: %klee -output-dir=%t-dfs-bfs.klee-out -search=dfs -search=bfs %t.bc >%t-dfs-bfs.out
// RUN: FileCheck -input-file=%t-bfs.out %s
// RUN: FileCheck -input-file=%t-dfs.out %s
// RUN: FileCheck -input-file=%t-bfs-dfs.out %s
// RUN: FileCheck -input-file=%t-dfs-bfs.out %s

#include "klee/klee.h"

int main() {
  int x, y, z;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), "y");
  klee_make_symbolic(&z, sizeof(z), "z");

  if (x == 1) {
    if (y == 1) {
      if (z == 1) {
        printf("A");
      } else {
        printf("B");
      }
    }
  } else {
    if (y == 1) {
      if (z == 1) {
        printf("C");
      } else {
        printf("D");
      }
    }
  }

  // exactly 4 characters, each of A, B, C and D occur exactly once
  // CHECK: {{^(ABCD|ABDC|ACBD|ACDB|ADBC|ADCB|BACD|BADC|BCAD|BCDA|BDAC|BDCA|CABD|CADB|CBAD|CBDA|CDAB|CDBA|DABC|DACB|DBAC|DBCA|DCAB|DCBA)$}}
}
