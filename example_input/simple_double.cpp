/* simple_double.cpp
Тест на две ленивые инстанциации в одном объекте.
*/

#include "klee/klee.h"
// #include <iostream>

struct I {
  I *l, *r;
  int v;
};


int main() {
  I* root;
  klee_make_symbolic(&root, sizeof(root), "root");
  if(root->l == root->r) {
    return 0;
  } else {
    return -1;
  }
}
