// RUN: %llvmgcc -c -o %t1.bc %s
// RUN: %klee --exit-on-error %t1.bc

#include <stdio.h>

#define ADDRESS ((int*) 0x0080)

int main() {
  klee_define_fixed_object(ADDRESS, 4);
  
  int *p = ADDRESS;

  *p = 10;
  printf("*p: %d\n", *p);

  return 0;
}
