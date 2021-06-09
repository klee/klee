/* simple_array_access.cpp
Тест на массиве
*/

#include "klee/klee.h"
// #include <iostream>

struct A {
  int a;
};

int main() {
  A* lst[3];
  int counter = 0;
  klee_make_symbolic(&lst, sizeof(lst), "lst");
  for(int i = 0; i<3; i++) {
    if(lst[i]->a == 13) counter++; 
  }
  if(counter == 3) return 0;
  else return 1;
}
