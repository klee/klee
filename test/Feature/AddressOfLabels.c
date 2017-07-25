// RUN: %llvmgcc -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out %t.bc 2> %t.log
// RUN: FileCheck --input-file %t.log %s

/* 
   This currently tests that KLEE clearly reports that it doesn't
   support taking the address of the labels, in particular
   blockaddress constants.

   The test would need to be changes once support for this feature is
   added. 
*/

#include <stdio.h>

int main (int argc, char** argv)
{
  int i = 1;
  // CHECK: Cannot handle constant 'i8* blockaddress 
  // CHECK: AddressOfLabels.c:[[@LINE+1]]
  void * lptr = &&one;

  if (argc > 1)
    lptr = &&two;
  
  goto *lptr;

 one:
  printf("argc is 1\n");
  return 0;
  
 two:
  printf("argc is >1\n");
  return 0;
}
