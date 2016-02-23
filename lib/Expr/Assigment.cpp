//===-- Assignment.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/util/Assignment.h"
namespace klee {

void Assignment::dump() {
  if (bindings.size() == 0) {
    llvm::errs() << "No bindings\n";
    return;
  }
  for (bindings_ty::iterator i = bindings.begin(), e = bindings.end(); i != e;
       ++i) {
    llvm::errs() << (*i).first->name << "\n[";
    for (int j = 0, k = (*i).second.size(); j < k; ++j)
      llvm::errs() << (int)(*i).second[j] << ",";
    llvm::errs() << "]\n";
  }
}
}
