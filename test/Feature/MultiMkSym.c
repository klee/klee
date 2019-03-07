// RUN: %clang -I../../../include -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-kqueries %t.bc > %t.log
// RUN: cat %t.klee-out/test000001.kquery %t.klee-out/test000002.kquery %t.klee-out/test000003.kquery %t.klee-out/test000004.kquery > %t1
// RUN: grep "a\[1\]" %t1 | wc -l | grep 2
// RUN: grep "a\[100\]" %t1 | wc -l | grep 2

/* Tests that the Array factory correctly distinguishes between arrays created at the same location but with different sizes */

#include <stdio.h>
#include <stdlib.h>

char* mk_sym(int size) {
  char *a = malloc(size);
  klee_make_symbolic(a, size, "a");
  return a;
}

int main() {
  int t;
  char *a, *b;

  klee_make_symbolic(&t, sizeof(t), "t");

  if (t) {
    printf("Allocate obj of size 1\n");
    a = mk_sym(1);
    if (a[0] > 'a')
      printf("Yes\n");
    else printf("No\n");
  }
  else {
    printf("Allocate obj of size 2\n");
    b = mk_sym(100);
    if (b[99] > 'a')
      printf("Yes\n");
    else printf("No\n");
  }

  return 0;
}
