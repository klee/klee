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
  // FIXME: What we need to check here instead is that _value is of the same
  // width as the range of the array that the update node is part of.
  /*
  assert(_value->getWidth() == Expr::Int8 &&
         "Update value should be 8-bit wide.");
  */
  computeHash();
  computeHeight();
  size = next ? next->size + 1 : 1;
}

extern "C" void vc_DeleteExpr(void *);

int UpdateNode::compare(const UpdateNode &b) const {
  if (int i = index.compare(b.index))
    return i;
  return value.compare(b.value);
}

bool UpdateNode::equals(const UpdateNode &b) const { return compare(b) == 0; }

unsigned UpdateNode::computeHash() {
  hashValue = index->hash() ^ value->hash();
  if (next)
    hashValue ^= next->hash();
  return hashValue;
}

unsigned UpdateNode::computeHeight() {
  unsigned maxHeight = next ? next->height() : 0;
  maxHeight = std::max(maxHeight, index->height());
  maxHeight = std::max(maxHeight, value->height());
  heightValue = maxHeight;
  return heightValue;
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
  if (root->source != b.root->source)
    return root->source < b.root->source ? -1 : 1;

  // Check the root itself in case we have separate objects with the
  // same name.
  if (root != b.root)
    return root < b.root ? -1 : 1;

  if (getSize() < b.getSize())
    return -1;
  else if (getSize() > b.getSize())
    return 1;

  // XXX build comparison into update, make fast
  const auto *an = head.get(), *bn = b.head.get();
  for (; an && bn; an = an->next.get(), bn = bn->next.get()) {
    if (an == bn) { // exploit shared list structure
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
  res = (res * Expr::MAGIC_HASH_CONSTANT) + root->source->hash();
  if (head)
    res = (res * Expr::MAGIC_HASH_CONSTANT) + head->hash();
  return res;
}

unsigned UpdateList::height() const { return head ? head->height() : 0; }
