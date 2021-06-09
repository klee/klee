/* hw4.cpp
Взята реализация AVL дерева студента CS-центра.
*/


#include "klee/klee.h"
// #include <iostream>

int max(int a, int b) {
  return (a > b ? a : b);
}

struct Node {
    int value;
    int height;
    Node* left;
    Node* right;

    Node(int v = 0, int h = 0, Node *l = nullptr, Node *r = nullptr) : value(v), left(l), right(r), height(h) {}
};

int height(Node*& root) {
    if (root == nullptr) return 0;
    return root->height;
}

int imbalance(Node* root) {
    return height(root->right) - height(root->left);
}

Node* rightRotation(Node*& root) {
    Node* tmp = root->left;
    root->left = tmp->right;
    tmp->right = root;
    root->height = max(height(root->left), height(root->right)) + 1;
    tmp->height = max(height(tmp->left), height(tmp->right)) + 1;
    return tmp;
}

Node* leftRotation(Node*& root) {
    Node* tmp = root->right;
    root->right = tmp->left;
    tmp->left = root;
    root->height = max(height(root->left), height(root->right)) + 1;
    tmp->height = max(height(tmp->left), height(tmp->right)) + 1;
    return tmp;
}

Node* balance(Node*& root) {
    if (root == nullptr) return nullptr;
    
    root->height = max(height(root->left), height(root->right)) + 1;
    if (imbalance(root) > 1) {
        if (imbalance(root->right) < 0) root->right = rightRotation(root->right);
        return leftRotation(root);
    }
    if (imbalance(root) < -1) {
        if (imbalance(root->left) > 0) root->left = leftRotation(root->left);
        return rightRotation(root);
    }
    return root;
}

Node* insert(Node*& root, int value) {
    if (root == nullptr) {
        root = new Node(value, 1);
        return root;
    }
    if (value < root->value)
        root->left = insert(root->left, value);
    else if (value > root->value)
        root->right = insert(root->right, value);
    else
        return root;
    root = balance(root);
    return root;
}

Node* minPointer(Node*& root) {
    if (root == nullptr) return nullptr;
    if (root->left != nullptr) return minPointer(root->left);
    return root;
}

Node* delMin(Node*& root) {
    if (root == nullptr) return nullptr;
    if (root->left == nullptr) return root->right;

    root->left = delMin(root->left);
    root = balance(root);
    return root;
}

// void treeoutput(Node*& root) {
//   if(root == nullptr) return;
//   std::cout << root->value << " ";
//   treeoutput(root->left);
//   treeoutput(root->right);
//   return;
// }

Node* deleteElem(Node*& root, int value) {
    if (root == nullptr) return nullptr;

    if (value < root->value) root->left = deleteElem(root->left, value);
    else if (value > root->value) root->right = deleteElem(root->right, value);
    else {
        Node* y = root->left;
        Node* z = root->right;
        if (z == nullptr) return y;

        Node* min = minPointer(z);
        min->right = delMin(z);
        min->left = y;
        return balance(min);
    }
    return balance(root);
}

Node* find(Node*& root, int value) {
    if (root == nullptr || root->value == value)
        return root;
    else if (root->value < value)
        return find(root->right, value);
    return find(root->left, value);
}

bool exist(Node*& root, int value) {
    return find(root, value) != nullptr;
}

Node* min(Node*& root) {
    if (root != nullptr && root->left != nullptr) return min(root->left);
    return root;
}

Node* max(Node*& root) {
    if (root != nullptr && root->right != nullptr) return max(root->right);
    return root;
}

Node* next(Node*& root, int value) {
    Node* x = find(root, value);
    bool key = false;
    if (x == nullptr) {
        key = true;
        root = insert(root, value);
        x = find(root, value);
    }
    if (x->right != nullptr) {
        Node* tmp = min(x->right);
        if (key) {
            root = deleteElem(root, value);
        }
        return tmp;
    }
    Node* succ = nullptr;
    Node* tmp = root;
    while (tmp != nullptr) {
        if (x->value < tmp->value) {
            succ = tmp;
            tmp = tmp->left;
        } else if (x->value > tmp->value) {
            tmp = tmp->right;
        } else {
            break;
        }
    }
    if (key) {
        root = deleteElem(root, value);
    }
    return succ;
}

Node* prev(Node*& root, int value) {
    Node* x = find(root, value);
    bool key = false;
    if (x == nullptr) {
        key = true;
        root = insert(root, value);
        x = find(root, value);
    }
    if (x->left != nullptr) {
        Node* tmp = max(x->left);
        if (key) root = deleteElem(root, value);
        return tmp;
    }
    Node* succ = nullptr;
    Node* tmp = root;
    while (tmp != nullptr) {
        if (x->value > tmp->value) {
            succ = tmp;
            tmp = tmp->right;
        } else if (x->value < tmp->value)
            tmp = tmp->left;
        else
            break;
    }
    if (key == true) root = deleteElem(root, value);
    return succ;
}

class AVLTree {
public:
    Node *root_;
    AVLTree(Node *root) : root_(root) {}
    ~AVLTree() = default;

    void insertT(int key) {
        root_ = insert(root_, key);
    }
    bool existsT(int key) {
        if (exist(root_, key))
	  return true;
        else
	  return false;
    } 
    int nextT(int key) {
        if (auto ans = next(root_, key))
	  return ans->value;
        else
	  return 0;
    }
    int prevT(int key) {
        if (auto ans = prev(root_, key))
	  return ans->value;
        else
          return 0;
    }
    void deleteT(int key) {
        root_ = deleteElem(root_, key);
    }   
};


int main() {
    int key;
    klee_make_symbolic(&key, sizeof(key), "key");
    AVLTree *tree;
    klee_make_symbolic(&tree, sizeof(tree), "tree");
    int result = tree->existsT(key);
    // treeoutput(tree->root_);
    return result;
}
