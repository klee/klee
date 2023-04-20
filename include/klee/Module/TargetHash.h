//===-- TargetHash.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TARGETHASH_H
#define KLEE_TARGETHASH_H

#include "klee/ADT/Ref.h"

#include <map>
#include <unordered_map>

namespace llvm {
class BasicBlock;
}

namespace klee {
struct Target;

struct TargetHash {
  unsigned operator()(const Target *t) const;
};

struct TargetCmp {
  bool operator()(const Target *a, const Target *b) const;
};

struct EquivTargetCmp {
  bool operator()(const Target *a, const Target *b) const;
};

struct RefTargetHash {
  unsigned operator()(const ref<Target> &t) const;
};

struct RefTargetCmp {
  bool operator()(const ref<Target> &a, const ref<Target> &b) const;
};

typedef std::pair<llvm::BasicBlock *, llvm::BasicBlock *> Transition;

struct TransitionHash {
  std::size_t operator()(const Transition &p) const;
};

struct RefTargetLess {
  bool operator()(const ref<Target> &a, const ref<Target> &b) const {
    return a.get() < b.get();
  }
};

} // namespace klee
#endif /* KLEE_TARGETHASH_H */
