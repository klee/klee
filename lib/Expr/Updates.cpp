//===-- Updates.cpp -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Expr.h"

#include <cassert>

using namespace klee;

UpdateNode::UpdateNode(const UpdateNode *_next, 
                       const ref<Expr> &_index, 
                       const ref<Expr> &_value) 
  : refCount(0),    
    next(_next),
    index(_index),
    value(_value) {
  // FIXME: What we need to check here instead is that _value is of the same width 
  // as the range of the array that the update node is part of.
  /*
  assert(_value->getWidth() == Expr::Int8 && 
         "Update value should be 8-bit wide.");
  */
  computeHash();
  if (next) {
    ++next->refCount;
    size = 1 + next->size;
  }
  else size = 1;
}

extern "C" void vc_DeleteExpr(void*);

// This is deliberately empty to avoid recursively deleting UpdateNodes
// (i.e. ``if (--next->refCount==0) delete next;``).
// UpdateList manages the correct destruction of a chain UpdateNodes
// non-recursively.
UpdateNode::~UpdateNode() {
    assert(refCount == 0 && "Deleted UpdateNode when a reference is still held");
}

int UpdateNode::compare(const UpdateNode &b) const {
  if (int i = index.compare(b.index)) 
    return i;
  return value.compare(b.value);
}

unsigned UpdateNode::computeHash() {
  hashValue = index->hash() ^ value->hash();
  if (next)
    hashValue ^= next->hash();
  return hashValue;
}

///

UpdateList::UpdateList(const Array *_root, const UpdateNode *_head)
  : root(_root),
    head(_head) {
  if (head) ++head->refCount;
}

UpdateList::UpdateList(const UpdateList &b)
  : root(b.root),
    head(b.head) {
  if (head) ++head->refCount;
}

UpdateList::~UpdateList() {
    tryFreeNodes();
}

void UpdateList::tryFreeNodes() {
  // We need to be careful and avoid having the UpdateNodes recursively deleting
  // themselves. This is done in cooperation with the private dtor of UpdateNode
  // which does nothing.
  //
  // This method will free UpdateNodes that only this instance has references
  // to, i.e. it will delete a continguous chain of UpdateNodes that all have
  // a ``refCount`` of 1 starting at the head.
  //
  // In the following examples the Head0 is the head of this UpdateList instance
  // and Head1 is the head of some other instance of UpdateList.
  //
  // [x] represents an UpdateNode where the reference count for that node is x.
  //
  // Example: Simple chain.
  //
  // [1] -> [1] -> [1] -> nullptr
  //  ^Head0
  //
  // Result: The entire chain is freed
  //
  // nullptr
  // ^Head0
  //
  // Example: A chain where two UpdateLists share some nodes
  //
  // [1] -> [1] -> [2] -> [1] -> nullptr
  //  ^Head0        ^Head1
  //
  // Result: Part of the chain is freed and the UpdateList with head Head1
  //         is now the exclusive owner of the UpdateNodes.
  //
  // nullptr       [1] -> [1] -> nullptr
  //  ^Head0        ^Head1
  //
  // Example: A chain where two UpdateLists point at the same head
  //
  // [2] -> [1] -> [1] -> [1] -> nullptr
  //  ^Head0
  //   Head1
  //
  // Result: No UpdateNodes are freed but now the UpdateList with head Head1
  //         is now the exclusive owner of the UpdateNodes.
  //
  // [1] -> [1] -> [1] -> [1] -> nullptr
  //  ^Head1
  //
  //  nullptr
  //  ^Head0
  //
  while (head && --head->refCount==0) {
    const UpdateNode *n = head->next;
    delete head;
    head = n;
  }
}

UpdateList &UpdateList::operator=(const UpdateList &b) {
  if (b.head) ++b.head->refCount;
  // Drop reference to the current head and free a chain of nodes
  // if we are the only UpdateList referencing them
  tryFreeNodes();
  root = b.root;
  head = b.head;
  return *this;
}

void UpdateList::extend(const ref<Expr> &index, const ref<Expr> &value) {
  
  if (root) {
    assert(root->getDomain() == index->getWidth());
    assert(root->getRange() == value->getWidth());
  }

  if (head) --head->refCount;
  head = new UpdateNode(head, index, value);
  ++head->refCount;
}

int UpdateList::compare(const UpdateList &b) const {
  if (root->name != b.root->name)
    return root->name < b.root->name ? -1 : 1;

  // Check the root itself in case we have separate objects with the
  // same name.
  if (root != b.root)
    return root < b.root ? -1 : 1;

  if (getSize() < b.getSize()) return -1;
  else if (getSize() > b.getSize()) return 1;    

  // XXX build comparison into update, make fast
  const UpdateNode *an=head, *bn=b.head;
  for (; an && bn; an=an->next,bn=bn->next) {
    if (an==bn) { // exploit shared list structure
      return 0;
    } else {
      if (int res = an->compare(*bn))
        return res;
    }
  }
  assert(!an && !bn);  
  return 0;
}

unsigned UpdateList::hash() const {
  unsigned res = 0;
  for (unsigned i = 0, e = root->name.size(); i != e; ++i)
    res = (res * Expr::MAGIC_HASH_CONSTANT) + root->name[i];
  if (head)
    res ^= head->hash();
  return res;
}
