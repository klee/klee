//===-- Assignment.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Assignment.h"

#include "klee/Expr/Constraints.h"

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

ConstraintSet Assignment::createConstraintsFromAssignment() const {
  ConstraintSet result;
  for (const auto &binding : bindings) {
    const auto &array = binding.first;
    const auto &values = binding.second;

    if (array->getSize() <= 8 && array->getRange() == Expr::Int8) {
      ref<Expr> e =
          Expr::createTempRead(array, array->getSize() * array->getRange());
      result.push_back(EqExpr::create(e, evaluate(e)));
    } else {
      for (unsigned arrayIndex = 0; arrayIndex < array->size; ++arrayIndex) {
        unsigned char value = values[arrayIndex];
        result.push_back(EqExpr::create(
            ReadExpr::create(
                UpdateList(array, 0),
                ConstantExpr::alloc(arrayIndex, array->getDomain())),
            ConstantExpr::alloc(value, array->getRange())));
      }
    }
  }
  return result;
}

std::vector<const Array *> Assignment::getArrays() {
  std::vector<const Array *> arrays;
  arrays.reserve(bindings.size());
  for (auto i : bindings) {
    arrays.push_back(i.first);
  }
  return arrays;
}

} // namespace klee
