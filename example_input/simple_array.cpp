/* simple_array.cpp
Тест на массиве
*/

#include "klee/klee.h"
// #include <iostream>

struct A {
  int a;
};


int main() {
  A* lst[10];
  klee_make_symbolic(&lst, sizeof(lst), "lst");
  if(lst[3]->a == lst[5]->a && lst[3]->a == 8) return 0;
  else return 1;
}
