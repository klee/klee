// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -f override.config
// RUN: rm -rf %t.klee-out
// RUN: ln -s %S/bad.config override.config
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t1.bc
// RUN: not %klee-stats %t.klee-out

//Malformed config, klee-stats should error
#include <stdio.h>

int ackermann(int m, int n) {
   if (m == 0)
     return n+1;
   else
     return ackermann(m-1, (n==0) ? 1 : ackermann(m, n-1));
 }

int main() {
  int a = ackermann(2,2);
  if(a > 0)
    return 1;
  else 
    return 0;
}
