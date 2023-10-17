#include "klee/Expr/SymbolicSource.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
DISABLE_WARNING_POP

#include <vector>

namespace klee {
int SymbolicSource::compare(const SymbolicSource &b) const {
  return internalCompare(b);
}

bool SymbolicSource::equals(const SymbolicSource &b) const {
  return compare(b) == 0;
}

void SymbolicSource::print(llvm::raw_ostream &os) const {
  ExprPPrinter::printSignleSource(os, const_cast<SymbolicSource *>(this));
}

void SymbolicSource::dump() const {
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

std::string SymbolicSource::toString() const {
  std::string str;
  llvm::raw_string_ostream output(str);
  this->print(output);
  return str;
}

std::set<const Array *> LazyInitializationSource::getRelatedArrays() const {
  std::vector<const Array *> objects;
  findObjects(pointer, objects);
  return std::set<const Array *>(objects.begin(), objects.end());
}

unsigned ConstantSource::computeHash() {
  auto defaultV = constantValues.defaultV();
  auto ordered = constantValues.calculateOrderedStorage();

  unsigned res = (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) +
                 (defaultV ? defaultV->hash() : 0);

  for (auto kv : ordered) {
    res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + kv.first;
    res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + kv.second->hash();
  }

  hashValue = res;
  return hashValue;
}

unsigned UninitializedSource::computeHash() {
  unsigned res = getKind();
  res = res * SymbolicSource::MAGIC_HASH_CONSTANT + version;
  res = res * SymbolicSource::MAGIC_HASH_CONSTANT + allocSite->hash();
  hashValue = res;
  return hashValue;
}

int UninitializedSource::internalCompare(const SymbolicSource &b) const {
  if (getKind() != b.getKind()) {
    return getKind() < b.getKind() ? -1 : 1;
  }
  const UninitializedSource &ub = static_cast<const UninitializedSource &>(b);

  if (version != ub.version) {
    return version < ub.version ? -1 : 1;
  }

  if (allocSite != ub.allocSite) {
    return allocSite->getGlobalIndex() < ub.allocSite->getGlobalIndex() ? -1
                                                                        : 1;
  }

  return 0;
}

int SymbolicSizeConstantAddressSource::internalCompare(
    const SymbolicSource &b) const {
  if (getKind() != b.getKind()) {
    return getKind() < b.getKind() ? -1 : 1;
  }
  const SymbolicSizeConstantAddressSource &ub =
      static_cast<const SymbolicSizeConstantAddressSource &>(b);

  if (version != ub.version) {
    return version < ub.version ? -1 : 1;
  }
  if (size != ub.size) {
    return size < ub.size ? -1 : 1;
  }
  if (allocSite != ub.allocSite) {
    return allocSite->getGlobalIndex() < ub.allocSite->getGlobalIndex() ? -1
                                                                        : 1;
  }

  return 0;
}

unsigned SymbolicSizeConstantAddressSource::computeHash() {
  unsigned res = getKind();
  res = res * MAGIC_HASH_CONSTANT + version;
  res = res * MAGIC_HASH_CONSTANT + allocSite->hash();
  res = res * MAGIC_HASH_CONSTANT + size->hash();
  hashValue = res;
  return hashValue;
}

unsigned MakeSymbolicSource::computeHash() {
  unsigned res = (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + version;
  for (unsigned i = 0, e = name.size(); i != e; ++i) {
    res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + name[i];
  }
  hashValue = res;
  return hashValue;
}

unsigned LazyInitializationSource::computeHash() {
  unsigned res =
      (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + pointer->hash();
  hashValue = res;
  return hashValue;
}

int ArgumentSource::internalCompare(const SymbolicSource &b) const {
  if (getKind() != b.getKind()) {
    return getKind() < b.getKind() ? -1 : 1;
  }
  const ArgumentSource &ab = static_cast<const ArgumentSource &>(b);
  if (index != ab.index) {
    return index < ab.index ? -1 : 1;
  }
  assert(km == ab.km);
  auto parent = allocSite.getParent();
  auto bParent = ab.allocSite.getParent();
  if (km->getFunctionId(parent) != km->getFunctionId(bParent)) {
    return km->getFunctionId(parent) < km->getFunctionId(bParent) ? -1 : 1;
  }
  if (allocSite.getArgNo() != ab.allocSite.getArgNo()) {
    return allocSite.getArgNo() < ab.allocSite.getArgNo() ? -1 : 1;
  }
  return 0;
}

int InstructionSource::internalCompare(const SymbolicSource &b) const {
  if (getKind() != b.getKind()) {
    return getKind() < b.getKind() ? -1 : 1;
  }
  const InstructionSource &ib = static_cast<const InstructionSource &>(b);
  if (index != ib.index) {
    return index < ib.index ? -1 : 1;
  }
  assert(km == ib.km);
  auto function = allocSite.getParent()->getParent();
  auto kf = km->functionMap.at(function);
  auto bfunction = ib.allocSite.getParent()->getParent();
  auto bkf = km->functionMap.at(bfunction);

  if (kf->instructionMap[&allocSite]->getGlobalIndex() !=
      bkf->instructionMap[&ib.allocSite]->getGlobalIndex()) {
    return kf->instructionMap[&allocSite]->getGlobalIndex() <
                   bkf->instructionMap[&ib.allocSite]->getGlobalIndex()
               ? -1
               : 1;
  }
  return 0;
}

unsigned ArgumentSource::computeHash() {
  unsigned res = (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + index;
  auto parent = allocSite.getParent();
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + km->getFunctionId(parent);
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + allocSite.getArgNo();
  hashValue = res;
  return hashValue;
}

unsigned InstructionSource::computeHash() {
  unsigned res = (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + index;
  auto function = allocSite.getParent()->getParent();
  auto kf = km->functionMap.at(function);
  auto block = allocSite.getParent();
  res =
      (res * SymbolicSource::MAGIC_HASH_CONSTANT) + km->getFunctionId(function);
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) +
        kf->blockMap[block]->getId();
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) +
        kf->instructionMap[&allocSite]->getIndex();
  hashValue = res;
  return hashValue;
}

} // namespace klee
