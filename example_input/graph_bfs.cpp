/* graph_bfs.cpp
Структура Node представляет собой вершину с максимум 4мя выходными ребрами.
Сама программа делает что-то вроде обхода в ширину ориентированного графа.
*/

#include "klee/klee.h"
// #include <iostream>

struct Node {
  int value;
  Node* nodes[4];
};

Node *arr[100];
int counter1 = 0;
int counter2 = 0;

void bfs() {
  int b;
  auto now = arr[counter2];
  if(counter2 >= 10) return;
  for(int i = 0; i<4; i++) {
    if(counter1 < 10 && now->nodes[i]) {
      arr[counter1++] = now->nodes[i];
    }
    if(i == 0) {
      b = 3;
    }
    if(i == 1) {
      b = -2;
    }
    if(i == 2) {
      b = 5;
    }
    if(i == 3) {
      b = 10;
    }
  }
  counter2++;
  bfs();
}

int main() {
  Node* node;
  klee_make_symbolic(&node, sizeof(node), "node");
  arr[0] = node;
  bfs();
  return 0;
}
