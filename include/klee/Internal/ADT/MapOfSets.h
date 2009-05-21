//===-- MapOfSets.h ---------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __UTIL_MAPOFSETS_H__
#define __UTIL_MAPOFSETS_H__

#include <cassert>
#include <vector>
#include <set>
#include <map>

// This should really be broken down into TreeOfSets on top of which
// SetOfSets and MapOfSets are easily implemeted. It should also be
// parameterized on the underlying set type. Neither are hard to do,
// just not worth it at the moment.
//
// I also broke some STLish conventions because I was bored.

namespace klee {

  /** This implements the UBTree data structure (see Hoffmann and
      Koehler, "A New Method to Index and Query Sets", IJCAI 1999) */
  template<class K, class V>
  class MapOfSets {
  public:
    class iterator;

  public:
    MapOfSets();

    void clear();

    void insert(const std::set<K> &set, const V &value);

    V *lookup(const std::set<K> &set);

    iterator begin();
    iterator end();

    void subsets(const std::set<K> &set, 
                 std::vector< std::pair<std::set<K>, V> > &resultOut);
    void supersets(const std::set<K> &set, 
                   std::vector< std::pair<std::set<K>, V> > &resultOut);
    
    template<class Predicate>
    V *findSuperset(const std::set<K> &set, const Predicate &p);
    template<class Predicate>
    V *findSubset(const std::set<K> &set, const Predicate &p);

  private:
    class Node;

    Node root;

    template<class Iterator, class Vector>
    void findSubsets(Node *n, 
                     const std::set<K> &accum,
                     Iterator begin, 
                     Iterator end,
                     Vector &resultsOut);
    template<class Iterator, class Vector>
    void findSupersets(Node *n, 
                       const std::set<K> &accum,
                       Iterator begin, 
                       Iterator end,
                       Vector &resultsOut);
    template<class Predicate>
    V *findSuperset(Node *n, 
                    typename std::set<K>::iterator begin, 
                    typename std::set<K>::iterator end,
                    const Predicate &p);
    template<class Predicate>
    V *findSubset(Node *n, 
                  typename std::set<K>::iterator begin, 
                  typename std::set<K>::iterator end,
                  const Predicate &p);
  };

  /***/

  template<class K, class V>
  class MapOfSets<K,V>::Node {
    friend class MapOfSets<K,V>;
    friend class MapOfSets<K,V>::iterator;

  public:
    typedef std::map<K, Node> children_ty;

    V value;

  private:
    bool isEndOfSet;
    std::map<K, Node> children;
    
  public:
    Node() : isEndOfSet(false) {}
  };
  
  template<class K, class V>
  class MapOfSets<K,V>::iterator {
    typedef std::vector< typename std::map<K, Node>::iterator > stack_ty;
    friend class MapOfSets<K,V>;
  private:
    Node *root;
    bool onEntry;
    stack_ty stack;

    void step() {
      if (onEntry) {
        onEntry = false;
        Node *n = stack.empty() ? root : &stack.back()->second;
        while (!n->children.empty()) {
          stack.push_back(n->children.begin());
          n = &stack.back()->second;
          if (n->isEndOfSet) {
            onEntry = true;
            return;
          }
        } 
      }

      while (!stack.empty()) {
        unsigned size = stack.size();
        Node *at = size==1 ? root : &stack[size-2]->second;
        typename std::map<K,Node>::iterator &cur = stack.back();
        ++cur;
        if (cur==at->children.end()) {
          stack.pop_back();
        } else {
          Node *n = &cur->second;

          while (!n->isEndOfSet) {
            assert(!n->children.empty());
            stack.push_back(n->children.begin());
            n = &stack.back()->second;
          }

          onEntry = true;
          break;
        }
      } 
    }

  public:
    // end()
    iterator() : onEntry(false) {} 
    // begin()
    iterator(Node *_n) : root(_n), onEntry(true) {
      if (!root->isEndOfSet)
        step();
    }

    const std::pair<const std::set<K>, const V> operator*() {
      assert(onEntry || !stack.empty());
      std::set<K> s;
      for (typename stack_ty::iterator it = stack.begin(), ie = stack.end();
           it != ie; ++it)
        s.insert((*it)->first);
      Node *n = stack.empty() ? root : &stack.back()->second;
      return std::make_pair(s, n->value);
    }

    bool operator==(const iterator &b) {
      return onEntry==b.onEntry && stack==b.stack;
    }
    bool operator!=(const iterator &b) {
      return !(*this==b);
    }

    iterator &operator++() {
      step();
      return *this;
    }
  };

  /***/

  template<class K, class V>
  MapOfSets<K,V>::MapOfSets() {}  

  template<class K, class V>
  void MapOfSets<K,V>::insert(const std::set<K> &set, const V &value) {
    Node *n = &root;
    for (typename std::set<K>::const_iterator it = set.begin(), ie = set.end();
         it != ie; ++it)
      n = &n->children.insert(std::make_pair(*it, Node())).first->second;
    n->isEndOfSet = true;
    n->value = value;
  }

  template<class K, class V>
  V *MapOfSets<K,V>::lookup(const std::set<K> &set) {
    Node *n = &root;
    for (typename std::set<K>::const_iterator it = set.begin(), ie = set.end();
         it != ie; ++it) {
      typename Node::children_ty::iterator kit = n->children.find(*it);
      if (kit==n->children.end()) {
        return 0;
      } else {
        n = &kit->second;
      }
    }
    if (n->isEndOfSet) {
      return &n->value;
    } else {
      return 0;
    }
  }

  template<class K, class V>
  typename MapOfSets<K,V>::iterator 
  MapOfSets<K,V>::begin() { return iterator(&root); }
  
  template<class K, class V>
  typename MapOfSets<K,V>::iterator 
  MapOfSets<K,V>::end() { return iterator(); }

  template<class K, class V>
  template<class Iterator, class Vector>
  void MapOfSets<K,V>::findSubsets(Node *n, 
                                  const std::set<K> &accum,
                                  Iterator begin, 
                                  Iterator end,
                                  Vector &resultsOut) {
    if (n->isEndOfSet) {
      resultsOut.push_back(std::make_pair(accum, n->value));
    }
    
    for (Iterator it=begin; it!=end;) {
      K elt = *it;
      typename Node::children_ty::iterator kit = n->children.find(elt);
      it++;
      if (kit!=n->children.end()) {
        std::set<K> nacc = accum;
        nacc.insert(elt);
        findSubsets(&kit->second, nacc, it, end, resultsOut);
      }
    }
  }

  template<class K, class V>
  void MapOfSets<K,V>::subsets(const std::set<K> &set,
                               std::vector< std::pair<std::set<K>, 
                                                      V> > &resultOut) {
    findSubsets(&root, std::set<K>(), set.begin(), set.end(), resultOut);
  }

  template<class K, class V>
  template<class Iterator, class Vector>
  void MapOfSets<K,V>::findSupersets(Node *n, 
                                     const std::set<K> &accum,
                                     Iterator begin, 
                                     Iterator end,
                                     Vector &resultsOut) {
    if (begin==end) {
      if (n->isEndOfSet)
        resultsOut.push_back(std::make_pair(accum, n->value));
      for (typename Node::children_ty::iterator it = n->children.begin(),
             ie = n->children.end(); it != ie; ++it) {
        std::set<K> nacc = accum;
        nacc.insert(it->first);
        findSupersets(&it->second, nacc, begin, end, resultsOut);
      }
    } else {
      K elt = *begin;
      Iterator next = begin;
      ++next;
      for (typename Node::children_ty::iterator it = n->children.begin(),
             ie = n->children.end(); it != ie; ++it) {
        std::set<K> nacc = accum;
        nacc.insert(it->first);
        if (elt==it->first) {
          findSupersets(&it->second, nacc, next, end, resultsOut);
        } else if (it->first<elt) {
          findSupersets(&it->second, nacc, begin, end, resultsOut);
        } else {
          break;
        }
      }
    }
  }

  template<class K, class V>
  void MapOfSets<K,V>::supersets(const std::set<K> &set,
                               std::vector< std::pair<std::set<K>, V> > &resultOut) {
    findSupersets(&root, std::set<K>(), set.begin(), set.end(), resultOut);
  }

  template<class K, class V>
  template<class Predicate>
  V *MapOfSets<K,V>::findSubset(Node *n, 
                                typename std::set<K>::iterator begin, 
                                typename std::set<K>::iterator end,
                                const Predicate &p) {   
    if (n->isEndOfSet && p(n->value)) {
      return &n->value;
    } else if (begin==end) {
      return 0;
    } else {
      typename Node::children_ty::iterator kend = n->children.end();
      typename Node::children_ty::iterator 
        kbegin = n->children.lower_bound(*begin);
      typename std::set<K>::iterator it = begin;
      if (kbegin==kend)
        return 0;
      for (;;) { // kbegin!=kend && *it <= kbegin->first
        while (*it < kbegin->first) {
          ++it;
          if (it==end)
            return 0;
        }
        if (*it == kbegin->first) {
          ++it;
          V *res = findSubset(&kbegin->second, it, end, p);
          if (res || it==end)
            return res;
        }
        while (kbegin->first < *it) {
          ++kbegin;
          if (kbegin==kend)
            return 0;
        }
      }
    }
  }
  
  template<class K, class V>
  template<class Predicate>
  V *MapOfSets<K,V>::findSuperset(Node *n, 
                                  typename std::set<K>::iterator begin, 
                                  typename std::set<K>::iterator end,
                                  const Predicate &p) {   
    if (begin==end) {
      if (n->isEndOfSet && p(n->value))
        return &n->value;
      for (typename Node::children_ty::iterator it = n->children.begin(),
             ie = n->children.end(); it != ie; ++it) {
        V *res = findSuperset(&it->second, begin, end, p);
        if (res) return res;
      }
    } else {
      typename Node::children_ty::iterator kbegin = n->children.begin();
      typename Node::children_ty::iterator kmid = 
        n->children.lower_bound(*begin);
      for (typename Node::children_ty::iterator it = n->children.begin(),
             ie = n->children.end(); it != ie; ++it) {
        V *res = findSuperset(&it->second, begin, end, p);
        if (res) return res;
      }
      if (kmid!=n->children.end() && *begin==kmid->first) {
        V *res = findSuperset(&kmid->second, ++begin, end, p);
        if (res) return res;
      }
    }
    return 0;
  }

  template<class K, class V>
  template<class Predicate>
  V *MapOfSets<K,V>::findSuperset(const std::set<K> &set, const Predicate &p) {    
    return findSuperset(&root, set.begin(), set.end(), p);
  }

  template<class K, class V>
  template<class Predicate>
  V *MapOfSets<K,V>::findSubset(const std::set<K> &set, const Predicate &p) {    
    return findSubset(&root, set.begin(), set.end(), p);
  }

  template<class K, class V>
  void MapOfSets<K,V>::clear() {
    root.isEndOfSet = false;
    root.value = V();
    root.children.clear();
  }

}

#endif
