//===-- TargetHash.c-------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/TargetHash.h"

#include "klee/Module/Target.h"

using namespace llvm;
using namespace klee;

unsigned TargetHash::operator()(const Target *t) const {
  if (t) {
    return t->hash();
  } else {
    return 0;
  }
}

bool TargetCmp::operator()(const Target *a, const Target *b) const {
  return a == b;
}

bool EquivTargetCmp::operator()(const Target *a, const Target *b) const {
  if (a == NULL || b == NULL)
    return false;
  return a->compare(*b) == 0;
}

unsigned RefTargetHash::operator()(const ref<Target> &t) const {
  return t->hash();
}

bool RefTargetCmp::operator()(const ref<Target> &a,
                              const ref<Target> &b) const {
  return a.get() == b.get();
}

std::size_t TransitionHash::operator()(const Transition &p) const {
  return reinterpret_cast<size_t>(p.first) * 31 +
         reinterpret_cast<size_t>(p.second);
}
