// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --max-stp-time=1 %t1.bc
#include <stdio.h>

int main() {
  long long int x, y = 102*75678 + 78, i = 101;

  klee_make_symbolic(&x, sizeof(x), "x");

  if (x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x + (x*x % (x+12)) == y*y*y*y*y*y*y*y*y*y*y*y*y*y*y*y % i)
    printf("Yes\n");
  else printf("No\n");
  
  return 0;
}
