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

///

UpdateNode::UpdateNode(const ref<UpdateNode> &_next, const ref<Expr> &_index,
                       const ref<Expr> &_value)
    : next(_next), index(_index), value(_value) {
  // FIXME: What we need to check here instead is that _value is of the same width 
  // as the range of the array that the update node is part of.
  /*
  assert(_value->getWidth() == Expr::Int8 && 
         "Update value should be 8-bit wide.");
  */
  computeHash();
  size = next.isNull() ? 1 : 1 + next->size;
}

extern "C" void vc_DeleteExpr(void*);

int UpdateNode::compare(const UpdateNode &b) const {
  if (int i = index.compare(b.index)) 
    return i;
  return value.compare(b.value);
}

unsigned UpdateNode::computeHash() {
  hashValue = index->hash() ^ value->hash();
  if (!next.isNull())
    hashValue ^= next->hash();
  return hashValue;
}

///

UpdateList::UpdateList(const Array *_root, const ref<UpdateNode> &_head)
    : root(_root), head(_head) {}

void UpdateList::extend(const ref<Expr> &index, const ref<Expr> &value) {
  
  if (root) {
    assert(root->getDomain() == index->getWidth());
    assert(root->getRange() == value->getWidth());
  }

  head = new UpdateNode(head, index, value);
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
  const auto *an = head.get(), *bn = b.head.get();
  for (; an && bn; an = an->next.get(), bn = bn->next.get()) {
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
  if (head.get())
    res ^= head->hash();
  return res;
}
