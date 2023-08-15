//===---- ImmutableList.h ---------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_IMMUTABLELIST_H
#define KLEE_IMMUTABLELIST_H

#include <memory>
#include <vector>

namespace klee {

template <typename T> class ImmutableList {
  struct ImmutableListNode {
    std::shared_ptr<ImmutableListNode> prev;
    const size_t prev_len;
    std::vector<T> values;

    [[nodiscard]] size_t size() const { return prev_len + values.size(); }

    ImmutableListNode() : prev(nullptr), prev_len(0), values() {}

    explicit ImmutableListNode(const ImmutableList<T> &il)
        : prev_len(il.size()), values() {
      std::shared_ptr<ImmutableListNode> pr = il.node;
      while (pr && pr->values.empty()) {
        pr = pr->prev;
      }
      if (pr && pr->size()) {
        prev = pr;
      } else {
        prev = nullptr;
      }
    }
  };

  std::shared_ptr<ImmutableListNode> node;

public:
  [[nodiscard]] size_t size() const { return node ? node->size() : 0; }

  struct iterator {
    const ImmutableListNode *rootNode;
    std::unique_ptr<iterator> it;
    size_t get;

  public:
    explicit iterator(const ImmutableListNode *p)
        : rootNode(p), it(nullptr), get(0) {
      if (rootNode && rootNode->prev.get()) {
        it = std::make_unique<iterator>(rootNode->prev.get());
      }
    }

    bool operator==(const iterator &b) const {
      return rootNode == b.rootNode && get == b.get;
    }

    bool operator!=(const iterator &b) const { return !(*this == b); }

    iterator &operator++() {
      ++get;
      if (get < rootNode->prev_len) {
        it->operator++();
      }
      return *this;
    }

    const T &operator*() const {
      if (get < rootNode->prev_len) {
        return **it;
      }
      return rootNode->values[get - rootNode->prev_len];
    }

    const T &operator->() const { return **this; }
  };

  [[nodiscard]] iterator begin() const { return iterator(node.get()); }

  [[nodiscard]] iterator end() const {
    auto it = iterator(node.get());
    it.get = size();
    return it;
  }

  void push_back(T &&value) {
    if (!node) {
      node = std::make_shared<ImmutableListNode>();
    }
    node->values.push_back(std::move(value));
  }

  ImmutableList() : node(){};
  ImmutableList(const ImmutableList<T> &il)
      : node(std::make_shared<ImmutableListNode>(il)) {}
};

} // namespace klee

#endif /* KLEE_IMMUTABLELIST_H */
