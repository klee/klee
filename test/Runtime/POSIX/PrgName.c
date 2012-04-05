// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: %klee --posix-runtime --exit-on-error %t2.bc --sym-arg 10 >%t.log
// RUN: test -f klee-last/test000001.ktest
// RUN: test -f klee-last/test000002.ktest
// RUN: grep -q "No" %t.log
// RUN: grep -qv "Yes" %t.log


/* Simple test for argv[0] */

#include <stdio.h>

int f(int argc, char **argv) {

  if (argv[0][5] == '7')
    printf("Yes\n");
  else printf("No\n");

  printf("%c\n", argv[0][5]);

  if (argv[1][1] == 4)
    printf("4\n");
  printf("not 4\n");

  return 0;
}


int main(int argc, char **argv) {
  f(argc, argv);
}
