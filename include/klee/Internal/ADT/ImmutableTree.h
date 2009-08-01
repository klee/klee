//===-- ImmutableTree.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __UTIL_IMMUTABLETREE_H__
#define __UTIL_IMMUTABLETREE_H__

#include <cassert>
#include <vector>

namespace klee {
  template<class K, class V, class KOV, class CMP>
  class ImmutableTree {
  public:
    static size_t allocated;
    class iterator;

    typedef K key_type;
    typedef V value_type;
    typedef KOV key_of_value;
    typedef CMP key_compare;

  public:
    ImmutableTree();
    ImmutableTree(const ImmutableTree &s);
    ~ImmutableTree();

    ImmutableTree &operator=(const ImmutableTree &s);

    bool empty() const;

    size_t count(const key_type &key) const; // always 0 or 1
    const value_type *lookup(const key_type &key) const;

    // find the last value less than or equal to key, or null if
    // no such value exists
    const value_type *lookup_previous(const key_type &key) const;

    const value_type &min() const;
    const value_type &max() const;
    size_t size() const;

    ImmutableTree insert(const value_type &value) const;
    ImmutableTree replace(const value_type &value) const;
    ImmutableTree remove(const key_type &key) const;
    ImmutableTree popMin(value_type &valueOut) const;
    ImmutableTree popMax(value_type &valueOut) const;

    iterator begin() const;
    iterator end() const;
    iterator find(const key_type &key) const;
    iterator lower_bound(const key_type &key) const;
    iterator upper_bound(const key_type &key) const;

    static size_t getAllocated() { return allocated; }

  private:
    class Node;

    Node *node;
    ImmutableTree(Node *_node);
  };

  /***/

  template<class K, class V, class KOV, class CMP>
  class ImmutableTree<K,V,KOV,CMP>::Node {
  public:
    static Node terminator;
    Node *left, *right;
    value_type value;
    unsigned height, references;

  protected:
    Node(); // solely for creating the terminator node
    static Node *balance(Node *left, const value_type &value, Node *right);

  public:

    Node(Node *_left, Node *_right, const value_type &_value);
    ~Node();

    void decref();
    Node *incref();

    bool isTerminator();

    size_t size();
    Node *popMin(value_type &valueOut);
    Node *popMax(value_type &valueOut);
    Node *insert(const value_type &v);
    Node *replace(const value_type &v);
    Node *remove(const key_type &k);
  };

  // Should live somewhere else, this is a simple stack with maximum (dynamic)
  // size.
  template<typename T>
  class FixedStack {
    unsigned pos, max;
    T *elts;

  public:
    FixedStack(unsigned _max) : pos(0),
                                max(_max),
                                elts(new T[max]) {}
    FixedStack(const FixedStack &b) : pos(b.pos),
                                      max(b.max),
                                      elts(new T[b.max]) {
      std::copy(b.elts, b.elts+pos, elts);
    }
    ~FixedStack() { delete[] elts; }

    void push_back(const T &elt) { elts[pos++] = elt; }
    void pop_back() { --pos; }
    bool empty() { return pos==0; }
    T &back() { return elts[pos-1]; }


    FixedStack &operator=(const FixedStack &b) {
      assert(max == b.max); 
      pos = b.pos;
      std::copy(b.elts, b.elts+pos, elts);
      return *this;
    }

    bool operator==(const FixedStack &b) {
      return (pos == b.pos &&
              std::equal(elts, elts+pos, b.elts));
    }
    bool operator!=(const FixedStack &b) { return !(*this==b); }
  };

  template<class K, class V, class KOV, class CMP>
  class ImmutableTree<K,V,KOV,CMP>::iterator {
    friend class ImmutableTree<K,V,KOV,CMP>;
  private:
    Node *root; // so can back up from end
    FixedStack<Node*> stack;
    
  public:
    iterator(Node *_root, bool atBeginning) : root(_root->incref()),
                                              stack(root->height) {
      if (atBeginning) {
        for (Node *n=root; !n->isTerminator(); n=n->left)
          stack.push_back(n);
      }
    }
    iterator(const iterator &i) : root(i.root->incref()),
                                  stack(i.stack) {
    }
    ~iterator() {
      root->decref();
    }

    iterator &operator=(const iterator &b) {
      b.root->incref();
      root->decref();
      root = b.root;
      stack = b.stack;
      return *this;
    }

    const value_type &operator*() {
      Node *n = stack.back();
      return n->value;
    }

    const value_type *operator->() {
      Node *n = stack.back();
      return &n->value;
    }

    bool operator==(const iterator &b) {
      return stack==b.stack;
    }
    bool operator!=(const iterator &b) {
      return stack!=b.stack;
    }
    
    iterator &operator--() {
      if (stack.empty()) {
        for (Node *n=root; !n->isTerminator(); n=n->right)
          stack.push_back(n);
      } else {
        Node *n = stack.back();
        if (n->left->isTerminator()) {
          for (;;) {
            Node *prev = n;
            stack.pop_back();
            if (stack.empty()) {
              break;
            } else {
              n = stack.back();
              if (prev==n->right)
                break;
            }
          }
        } else {
          stack.push_back(n->left);
          for (n=n->left->right; !n->isTerminator(); n=n->right)
            stack.push_back(n);
        }
      }
      return *this;
    }

    iterator &operator++() {
      assert(!stack.empty());
      Node *n = stack.back();
      if (n->right->isTerminator()) {
        for (;;) {
          Node *prev = n;
          stack.pop_back();
          if (stack.empty()) {
            break;
          } else {
            n = stack.back();
            if (prev==n->left)
              break;
          }
        }
      } else {
        stack.push_back(n->right);
        for (n=n->right->left; !n->isTerminator(); n=n->left)
          stack.push_back(n);
      }
      return *this;
    }
  };

  /***/

  template<class K, class V, class KOV, class CMP> 
  typename ImmutableTree<K,V,KOV,CMP>::Node 
  ImmutableTree<K,V,KOV,CMP>::Node::terminator;

  template<class K, class V, class KOV, class CMP> 
  size_t ImmutableTree<K,V,KOV,CMP>::allocated = 0;

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP>::Node::Node() 
    : left(&terminator), 
      right(&terminator), 
      height(0), 
      references(3) { 
    assert(this==&terminator);
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP>::Node::Node(Node *_left, Node *_right, const value_type &_value)
    : left(_left), 
      right(_right), 
      value(_value), 
      height(std::max(left->height, right->height) + 1),
      references(1) 
  {
    ++allocated;
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP>::Node::~Node() {
    left->decref();
    right->decref();
    --allocated;
  }

  template<class K, class V, class KOV, class CMP>
  inline void ImmutableTree<K,V,KOV,CMP>::Node::decref() {
    --references;
    if (references==0) delete this;
  }

  template<class K, class V, class KOV, class CMP>
  inline typename ImmutableTree<K,V,KOV,CMP>::Node *ImmutableTree<K,V,KOV,CMP>::Node::incref() {
    ++references;
    return this;
  }

  template<class K, class V, class KOV, class CMP>
  inline bool ImmutableTree<K,V,KOV,CMP>::Node::isTerminator() {
    return this==&terminator;
  }

  /***/

  template<class K, class V, class KOV, class CMP>
  typename ImmutableTree<K,V,KOV,CMP>::Node *
  ImmutableTree<K,V,KOV,CMP>::Node::balance(Node *left, const value_type &value, Node *right) {
    if (left->height > right->height + 2) {
      Node *ll = left->left;
      Node *lr = left->right;
      if (ll->height >= lr->height) {
        Node *nlr = new Node(lr->incref(), right, value);
        Node *res = new Node(ll->incref(), nlr, left->value);
        left->decref();
        return res;
      } else {
        Node *lrl = lr->left;
        Node *lrr = lr->right;
        Node *nll = new Node(ll->incref(), lrl->incref(), left->value);
        Node *nlr = new Node(lrr->incref(), right, value);
        Node *res = new Node(nll, nlr, lr->value);
        left->decref();
        return res;
      }
    } else if (right->height > left->height + 2) {
      Node *rl = right->left;
      Node *rr = right->right;
      if (rr->height >= rl->height) {
        Node *nrl = new Node(left, rl->incref(), value);
        Node *res = new Node(nrl, rr->incref(), right->value);
        right->decref();
        return res;
      } else {
        Node *rll = rl->left;
        Node *rlr = rl->right;
        Node *nrl = new Node(left, rll->incref(), value);
        Node *nrr = new Node(rlr->incref(), rr->incref(), right->value);
        Node *res = new Node(nrl, nrr, rl->value);
        right->decref();
        return res;
      }
    } else {
      return new Node(left, right, value);
    }
  }

  template<class K, class V, class KOV, class CMP>
  size_t ImmutableTree<K,V,KOV,CMP>::Node::size() {
    if (isTerminator()) {
      return 0;
    } else {
      return left->size() + 1 + right->size();
    }
  }

  template<class K, class V, class KOV, class CMP>
  typename ImmutableTree<K,V,KOV,CMP>::Node *
  ImmutableTree<K,V,KOV,CMP>::Node::popMin(value_type &valueOut) {
    if (left->isTerminator()) {
      valueOut = value;
      return right->incref();
    } else {
      return balance(left->popMin(valueOut), value, right->incref());
    }
  }

  template<class K, class V, class KOV, class CMP>
  typename ImmutableTree<K,V,KOV,CMP>::Node *
  ImmutableTree<K,V,KOV,CMP>::Node::popMax(value_type &valueOut) {
    if (right->isTerminator()) {
      valueOut = value;
      return left->incref();
    } else {
      return balance(left->incref(), value, right->popMax(valueOut));
    }
  }

  template<class K, class V, class KOV, class CMP>
  typename ImmutableTree<K,V,KOV,CMP>::Node *
  ImmutableTree<K,V,KOV,CMP>::Node::insert(const value_type &v) {
    if (isTerminator()) {
      return new Node(terminator.incref(), terminator.incref(), v);
    } else {
      if (key_compare()(key_of_value()(v), key_of_value()(value))) {
        return balance(left->insert(v), value, right->incref());
      } else if (key_compare()(key_of_value()(value), key_of_value()(v))) {
        return balance(left->incref(), value, right->insert(v));
      } else {
        return incref();
      }
    }
  }

  template<class K, class V, class KOV, class CMP>
  typename ImmutableTree<K,V,KOV,CMP>::Node *
  ImmutableTree<K,V,KOV,CMP>::Node::replace(const value_type &v) {
    if (isTerminator()) {
      return new Node(terminator.incref(), terminator.incref(), v);
    } else {
      if (key_compare()(key_of_value()(v), key_of_value()(value))) {
        return balance(left->replace(v), value, right->incref());
      } else if (key_compare()(key_of_value()(value), key_of_value()(v))) {
        return balance(left->incref(), value, right->replace(v));
      } else {
        return new Node(left->incref(), right->incref(), v);
      }
    }
  }

  template<class K, class V, class KOV, class CMP>
  typename ImmutableTree<K,V,KOV,CMP>::Node *
  ImmutableTree<K,V,KOV,CMP>::Node::remove(const key_type &k) {
    if (isTerminator()) {
      return incref();
    } else {
      if (key_compare()(k, key_of_value()(value))) {
        return balance(left->remove(k), value, right->incref());
      } else if (key_compare()(key_of_value()(value), k)) {
        return balance(left->incref(), value, right->remove(k));
      } else {
        if (left->isTerminator()) {
          return right->incref();
        } else if (right->isTerminator()) {
          return left->incref();
        } else {
          value_type min;
          Node *nr = right->popMin(min);
          return balance(left->incref(), min, nr);
        }
      }
    }
  }

  /***/

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP>::ImmutableTree() 
    : node(Node::terminator.incref()) {
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP>::ImmutableTree(Node *_node) 
    : node(_node) {
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP>::ImmutableTree(const ImmutableTree &s) 
    : node(s.node->incref()) {
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP>::~ImmutableTree() {
    node->decref(); 
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP> &ImmutableTree<K,V,KOV,CMP>::operator=(const ImmutableTree &s) {
    Node *n = s.node->incref();
    node->decref();
    node = n;
    return *this;
  }

  template<class K, class V, class KOV, class CMP>
  bool ImmutableTree<K,V,KOV,CMP>::empty() const {
    return node->isTerminator();
  }

  template<class K, class V, class KOV, class CMP>
  size_t ImmutableTree<K,V,KOV,CMP>::count(const key_type &k) const {
    Node *n = node;
    while (!n->isTerminator()) {
      key_type key = key_of_value()(n->value);
      if (key_compare()(k, key)) {
        n = n->left;
      } else if (key_compare()(key, k)) {
        n = n->right;
      } else {
        return 1;
      }
    }
    return 0;
  }

  template<class K, class V, class KOV, class CMP>
  const typename ImmutableTree<K,V,KOV,CMP>::value_type *
  ImmutableTree<K,V,KOV,CMP>::lookup(const key_type &k) const {
    Node *n = node;
    while (!n->isTerminator()) {
      key_type key = key_of_value()(n->value);
      if (key_compare()(k, key)) {
        n = n->left;
      } else if (key_compare()(key, k)) {
        n = n->right;
      } else {
        return &n->value;
      }
    }
    return 0;
  }

  template<class K, class V, class KOV, class CMP>
  const typename ImmutableTree<K,V,KOV,CMP>::value_type *
  ImmutableTree<K,V,KOV,CMP>::lookup_previous(const key_type &k) const {
    Node *n = node;
    Node *result = 0;
    while (!n->isTerminator()) {
      key_type key = key_of_value()(n->value);
      if (key_compare()(k, key)) {
        n = n->left;
      } else if (key_compare()(key, k)) {
        result = n;
        n = n->right;
      } else {
        return &n->value;
      }
    }
    return result ? &result->value : 0;
  }

  template<class K, class V, class KOV, class CMP>
  const typename ImmutableTree<K,V,KOV,CMP>::value_type &
  ImmutableTree<K,V,KOV,CMP>::min() const { 
    Node *n = node;
    assert(!n->isTerminator());
    while (!n->left->isTerminator()) n = n->left;
    return n->value;
  }

  template<class K, class V, class KOV, class CMP>
  const typename ImmutableTree<K,V,KOV,CMP>::value_type &
  ImmutableTree<K,V,KOV,CMP>::max() const {
    Node *n = node;
    assert(!n->isTerminator());
    while (!n->right->isTerminator()) n = n->right;
    return n->value;
  }

  template<class K, class V, class KOV, class CMP>
  size_t ImmutableTree<K,V,KOV,CMP>::size() const {
    return node->size();
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP> 
  ImmutableTree<K,V,KOV,CMP>::insert(const value_type &value) const { 
    return ImmutableTree(node->insert(value)); 
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP> 
  ImmutableTree<K,V,KOV,CMP>::replace(const value_type &value) const { 
    return ImmutableTree(node->replace(value)); 
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP> 
  ImmutableTree<K,V,KOV,CMP>::remove(const key_type &key) const { 
    return ImmutableTree(node->remove(key)); 
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP> 
  ImmutableTree<K,V,KOV,CMP>::popMin(value_type &valueOut) const { 
    return ImmutableTree(node->popMin(valueOut)); 
  }

  template<class K, class V, class KOV, class CMP>
  ImmutableTree<K,V,KOV,CMP> 
  ImmutableTree<K,V,KOV,CMP>::popMax(value_type &valueOut) const { 
    return ImmutableTree(node->popMax(valueOut)); 
  }

  template<class K, class V, class KOV, class CMP>
  inline typename ImmutableTree<K,V,KOV,CMP>::iterator 
  ImmutableTree<K,V,KOV,CMP>::begin() const {
    return iterator(node, true);
  }

  template<class K, class V, class KOV, class CMP>
  inline typename ImmutableTree<K,V,KOV,CMP>::iterator 
  ImmutableTree<K,V,KOV,CMP>::end() const {
    return iterator(node, false);
  }

  template<class K, class V, class KOV, class CMP>
  inline typename ImmutableTree<K,V,KOV,CMP>::iterator 
  ImmutableTree<K,V,KOV,CMP>::find(const key_type &key) const {
    iterator end(node,false), it = lower_bound(key);
    if (it==end || key_compare()(key,key_of_value()(*it))) {
      return end;
    } else {
      return it;
    }
  }

  template<class K, class V, class KOV, class CMP>
  inline typename ImmutableTree<K,V,KOV,CMP>::iterator 
  ImmutableTree<K,V,KOV,CMP>::lower_bound(const key_type &k) const {
    // XXX ugh this doesn't have to be so ugly does it?
    iterator it(node,false);
    for (Node *root=node; !root->isTerminator();) {
      it.stack.push_back(root);
      if (key_compare()(k, key_of_value()(root->value))) {
        root = root->left;
      } else if (key_compare()(key_of_value()(root->value), k)) {
        root = root->right;
      } else {
        return it;
      }
    }
    // it is now beginning or first element < k
    if (!it.stack.empty()) {
      Node *last = it.stack.back();
      if (key_compare()(key_of_value()(last->value), k))
        ++it;
    }
    return it;
  }

  template<class K, class V, class KOV, class CMP>
  typename ImmutableTree<K,V,KOV,CMP>::iterator 
  ImmutableTree<K,V,KOV,CMP>::upper_bound(const key_type &key) const {
    iterator end(node,false),it = lower_bound(key);
    if (it!=end && 
        !key_compare()(key,key_of_value()(*it))) // no need to loop, no duplicates
      ++it;
    return it;
  }

}

#endif
