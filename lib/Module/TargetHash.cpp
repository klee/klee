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

unsigned TargetHash::operator()(const ref<Target> &t) const {
  return t->hash();
}

bool TargetCmp::operator()(const ref<Target> &a, const ref<Target> &b) const {
  return a == b;
}

std::size_t BranchHash::operator()(const Branch &p) const {
  return reinterpret_cast<size_t>(p.first) * 31 + p.second;
}
