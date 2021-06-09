/* simple.cpp
 */

#include "klee/klee.h"
// #include <iostream>

void f(int *a, int x) {
  if (a != 0) {
    *a = x;
    return;
  }
}

int main() {
  int *a;
  int x;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&x, sizeof(x), "x");
  f(a, x);
  if (a != 0 && *a == 5)
    return 1;
  else if(a == 0) {
    return 2;
  } else {
    return 0;
  }
}
