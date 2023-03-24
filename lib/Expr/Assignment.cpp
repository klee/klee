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

void Assignment::dump() const {
  if (bindings.size() == 0) {
    llvm::errs() << "No bindings\n";
    return;
  }
  for (auto const& [array, values] : bindings) {
    llvm::errs() << array->name << "\n[";
    for (int j = 0, k = values.size(); j < k; ++j)
      llvm::errs() << static_cast<int>(values[j]) << ",";
    llvm::errs() << "]\n";
  }
}

ConstraintSet Assignment::createConstraintsFromAssignment() const {
  ConstraintSet result;
  for (const auto &binding : bindings) {
    const auto &array = binding.first;
    const auto &values = binding.second;

    for (unsigned arrayIndex = 0; arrayIndex < array->size; ++arrayIndex) {
      unsigned char value = values[arrayIndex];
      result.push_back(EqExpr::create(
          ReadExpr::create(UpdateList(array, 0),
                           ConstantExpr::alloc(arrayIndex, array->getDomain())),
          ConstantExpr::alloc(value, array->getRange())));
    }
  }
  return result;
}
}
