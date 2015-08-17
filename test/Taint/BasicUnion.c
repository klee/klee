// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc


/* Basic tainting an int*/
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int i;
  union {
      int i;
      char c[sizeof (int)];
  } value;


  klee_set_taint(8, &value, sizeof value);

  klee_set_taint(1, &value.c[2], 1);

  klee_assert(klee_get_taint(&value, sizeof value) == 8|1);
}
