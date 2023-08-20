//===-- Assignment.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Assignment.h"

#include "klee/ADT/Ref.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Symcrete.h"

namespace klee {

void Assignment::dump() const {
  if (bindings.size() == 0) {
    llvm::errs() << "No bindings\n";
    return;
  }
  for (bindings_ty::iterator i = bindings.begin(), e = bindings.end(); i != e;
       ++i) {
    llvm::errs() << (*i).first->getName() << "\n[";
    for (int j = 0, k = (*i).second.size(); j < k; ++j)
      llvm::errs() << (int)(*i).second.load(j) << ",";
    llvm::errs() << "]\n";
  }
}

void Assignment::addIndependentAssignment(const Assignment &b) {
  for (auto it : b) {
    bindings.insert(it);
  }
}

constraints_ty Assignment::createConstraintsFromAssignment() const {
  constraints_ty result;
  for (const auto &binding : bindings) {
    const auto &array = binding.first;
    const auto &values = binding.second;
    ref<ConstantExpr> arrayConstantSize =
        dyn_cast<ConstantExpr>(evaluate(array->size));
    assert(arrayConstantSize &&
           "Size of symbolic array should be computed in assignment.");
    uint64_t arraySize = arrayConstantSize->getZExtValue();
    if (arraySize <= 8 && array->getRange() == Expr::Int8) {
      ref<Expr> e = Expr::createTempRead(array, arraySize * array->getRange());
      result.insert(EqExpr::create(e, evaluate(e)));
    } else {
      for (unsigned arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex) {
        unsigned char value = values.load(arrayIndex);
        result.insert(EqExpr::create(
            ReadExpr::create(
                UpdateList(array, 0),
                ConstantExpr::alloc(arrayIndex, array->getDomain())),
            ConstantExpr::alloc(value, array->getRange())));
      }
    }
  }
  return result;
}

Assignment Assignment::diffWith(const Assignment &other) const {
  Assignment diffAssignment;
  for (const auto &it : other) {
    if (bindings.count(it.first) == 0 || bindings.at(it.first) != it.second) {
      diffAssignment.bindings.insert(it);
    }
  }
  return diffAssignment;
}

std::vector<const Array *> Assignment::keys() const {
  std::vector<const Array *> result;
  result.reserve(bindings.size());
  for (auto i : bindings) {
    result.push_back(i.first);
  }
  return result;
}

std::vector<SparseStorage<unsigned char>> Assignment::values() const {
  std::vector<SparseStorage<unsigned char>> result;
  result.reserve(bindings.size());
  for (auto i : bindings) {
    result.push_back(i.second);
  }
  return result;
}

Assignment Assignment::part(const SymcreteOrderedSet &symcretes) const {
  Assignment ret;
  for (auto symcrete : symcretes) {
    for (auto array : symcrete->dependentArrays()) {
      if (bindings.find(array) != bindings.end()) {
        ret.bindings.insert({array, bindings.at(array)});
      }
    }
  }
  return ret;
}

} // namespace klee
