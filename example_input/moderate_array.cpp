/* moderate_array.cpp
Тест на массиве
*/

#include "klee/klee.h"
// #include <iostream>

struct A {
  int a;
  int b;
  int c;
};


int main() {
  A* lst[3];
  int counter = 0;
  klee_make_symbolic(&lst, sizeof(lst), "lst");
  if(lst[0]->a * lst[1]->b == lst[2]->c && lst[2]->c != 0) return 0;
  else return 1;
}
