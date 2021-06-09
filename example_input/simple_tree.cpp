/* simple_tree.cpp
Проверка двоичного дерева на сбалансированность.
*/


#include "klee/klee.h"
#include <iostream>

struct Node {
    int value;
    int height;
    Node* left;
    Node* right;

    Node(int v = 0, int h = 0, Node *l = nullptr, Node *r = nullptr) : value(v), left(l), right(r), height(h) {}
};

int max(int a, int b) {
  return (a > b ? a : b);
}

int check_height(Node* root) {
  if(!root->left && !root->right) {
    return 1;
  }
  return 1 + max(check_height(root->left),check_height(root->right));
}

bool check(Node* root) {
  std::cout << root->value << "\n";
  if(!root->left && !root->right) {
    return true;
  }
  if(root->left && root->right &&
     check_height(root->left) == check_height(root->right) &&
     check(root->left) && check(root->right)) {
    return true;
  }
  return false;
}

int main() {
  Node* root;
  klee_make_symbolic(&root, sizeof(root), "root");
  if(check(root) == true) {
    return 0;
  } else {
    return 1;
  }
}
