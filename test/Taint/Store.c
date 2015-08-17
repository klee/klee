// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc


/* this function test that taint is propagated through pointers
   here value_p points originally to value_w. when *value_p (that is value_w) 
   is changed to value_r, the taint of value_w should have the combined taints
   of value_p and value_r!
*/
#include<klee/klee.h>
int 
main (int argc, char *argv[])
{
  int value_w = 0x12345678;
  int *value_p = &value_w;
  int value_r = 0x1111111;

  klee_set_taint(0, &value_w, sizeof(value_w));
  klee_set_taint(1, &value_p, sizeof(value_p));
  klee_set_taint(2, &value_r, sizeof(value_r));

  *value_p = value_r;

  klee_assert(klee_get_taint(&value_w,sizeof(value_w))==3);
  return 0;
}
