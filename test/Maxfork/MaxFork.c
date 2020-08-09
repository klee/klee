// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: %klee --max-forks=1 %t1.bc

#include <stdio.h>

int bar(int a, int b, int c) {
  int d = b + c;
  if (d < 1) {
    if (b < 3) {
      if (b > 3)
        return 6;
      return 1;
    }
    if (a == 42) {
      return 2;
    }
    return 3;
  } else {
    if (c < 42) {
      return 4;
    }
    return 5;
  }
}

int main() {
  int a;
  int b;
  int c;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");
  klee_make_symbolic(&c, sizeof(c), "c");
  int d = bar(a, b, c);
  printf("%d\n", d);
  return 0;
}
