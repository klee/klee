// RUN: %llvmgcc -c -g %s -o %t.bc
// RUN: rm -rf %t.out
// RUN: %klee --output-dir=%t.out %t.bc "initial"
// RUN: test -f %t.out/test000001.ktest
// RUN: not test -f %t.out/test000002.ktest
// RUN: %klee --only-replay-seeds --named-seed-matching --seed-out %t.out/test000001.ktest %t.bc > %t.log
// RUN: grep -q "a==3" %t.log
// RUN: grep -q "b==4" %t.log
// RUN: grep -q "c==5" %t.log
// RUN: grep -q "x==6" %t.log

#include <string.h>
#include <stdio.h>

int main(int argc, char **argv) {
  int a, b, c, x;

  if (argc==2 && strcmp(argv[1], "initial") == 0) {
    klee_make_symbolic(&a, sizeof a, "a");
    klee_make_symbolic(&b, sizeof b, "b");
    klee_make_symbolic(&c, sizeof c, "c");
    klee_make_symbolic(&x, sizeof x, "a");

    klee_assume(a == 3);
    klee_assume(b == 4);
    klee_assume(c == 5);
    klee_assume(x == 6);
  } else {
    klee_make_symbolic(&a, sizeof a, "a");
    klee_make_symbolic(&c, sizeof c, "c");
    klee_make_symbolic(&b, sizeof b, "b");
    klee_make_symbolic(&x, sizeof x, "a");
  }

  if (a==3) printf("a==3\n");
  if (b==4) printf("b==4\n");
  if (c==5) printf("c==5\n");
  if (x==6) printf("x==6\n");

  return 0;
}
