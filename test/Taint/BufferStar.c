// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc




/* Buffer tainting*/
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  char * buffer = "AAAAAAAAAAAAAAAA";
  unsigned i;

  //printf ("Taint all 16 bytes of a buffer with TAINT_A\n");
  klee_set_taint (1, buffer, 16);

  klee_assert (1 == klee_get_taint (buffer, 16));
  for (i = 0; i < 16; i++)
    klee_set_taint (1 << i, buffer + i, 1);


  klee_assert (klee_get_taint (buffer, 16) == 0xffff);
  for (i = 0; i < 16; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == (1 << i) | 1);

  //printf ("Clear taint of the second half of the buffer\n");
  klee_set_taint (0, buffer + 8, 8);
  klee_assert (klee_get_taint (buffer, 16) == 0xff);
  for (i = 0; i < 8; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == (1 << i) | 1);
  for (i = 8; i < 16; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == 0);


  //printf ("Clear taint of the buffer\n");
  klee_set_taint (0, buffer, 16);
  klee_assert (klee_get_taint (buffer, 16) == 0);
  for (i = 0; i < 16; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == 0);

}

