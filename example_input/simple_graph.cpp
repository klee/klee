/* simple_graph.cpp
Поиск цикла в орграфе.
*/


#include "klee/klee.h"
// #include <iostream>


struct Node {
  int value;
  int nodes[4];
};

Node* node[100];
bool color[100];

bool check_cycle(Node* now) {
  
  for(int i = 0; i<4; i++) {
    klee_assume(now->nodes[i] >= 0);
    klee_assume(now->nodes[i] < 100);
    if(!node[now->nodes[i]]) {
      continue;
    }
    if(color[now->nodes[i]] == 1) {
      return true;
    } else {
      color[now->nodes[i]] = 1;
      bool res = check_cycle(node[now->nodes[i]]);
      if(res) return true;
    }
  }
  return false;
}

int main() {
  klee_make_symbolic(&node, sizeof(node), "node");
  if(check_cycle(node[0])) {
    return 0;
  } else {
    return 1;
  }
}
