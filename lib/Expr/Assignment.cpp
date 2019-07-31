//===-- Assignment.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Assignment.h"

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

void Assignment::createConstraintsFromAssignment(
    std::vector<ref<Expr> > &out) const {
  assert(out.size() == 0 && "out should be empty");
  for (bindings_ty::const_iterator it = bindings.begin(), ie = bindings.end();
       it != ie; ++it) {
    const Array *array = it->first;
    const std::vector<unsigned char> &values = it->second;
    for (unsigned arrayIndex = 0; arrayIndex < array->size; ++arrayIndex) {
      unsigned char value = values[arrayIndex];
      out.push_back(EqExpr::create(
          ReadExpr::create(UpdateList(array, 0),
                           ConstantExpr::alloc(arrayIndex, array->getDomain())),
          ConstantExpr::alloc(value, array->getRange())));
    }
  }
}
}
