// Compile: clang -I ../../include -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone MaxFork.c
// Run: klee --max-forks=1 MaxFork.bc
// Output: 5 1
// if change line 12 to b < 3, it will always output 5 6
#include <klee/klee.h>
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
