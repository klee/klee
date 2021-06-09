#include "klee/klee.h"
#include <iostream>
#include <vector>


int main() {
  std::vector<int> a;
  klee_make_symbolic(&a, sizeof(a),"a");
  if(!a.empty()) return 1;
  else return 0;
}
