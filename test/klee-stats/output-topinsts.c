// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out0
// RUN: rm -rf %t.klee-out1
// RUN: rm -rf %t.klee-out2
// RUN: %klee --output-dir=%t.klee-out0 --libc=klee --exit-on-error %t1.bc
// RUN: %klee --output-dir=%t.klee-out1 --libc=klee --exit-on-error %t1.bc
// RUN: %klee --output-dir=%t.klee-out2 --libc=klee --exit-on-error %t1.bc
// RUN: %klee-stats %t.klee-out0 %t.klee-out1 %t.klee-out2 --top-instructions | grep 'Top Instr'

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
