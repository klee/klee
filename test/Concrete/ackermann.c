// RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

#include <stdio.h>

int ackermann(int m, int n) {
   if (m == 0)
     return n+1;
   else
     return ackermann(m-1, (n==0) ? 1 : ackermann(m, n-1));
 }

int main() {
  printf("ackerman(%d, %d) = %d\n", 2, 2, ackermann(2, 2));

  return 0;
}
