// RUN: not %klee-stats --print-reltime

//Test meant to fail, no dirs specified
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
