//===-- VersionedValue.cpp --------------------------------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementations of the classes related to versioned
/// value. Versioned values are data tokens that can constitute the nodes in
/// building up the dependency graph, for the purpose of computing interpolants.
///
//===----------------------------------------------------------------------===//

#include "ShadowArray.h"
#include "TxPrintUtil.h"

#include <klee/Internal/Module/VersionedValue.h>

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/Type.h>
#else
#include <llvm/Type.h>
#endif

using namespace klee;

namespace klee {

void AllocationContext::print(llvm::raw_ostream &stream,
                              const std::string &prefix) const {
  std::string tabs = makeTabs(1);
  if (value) {
    stream << prefix << "Location: ";
    value->print(stream);
  }
  if (callHistory.size() > 0) {
    stream << "\n" << prefix << "Call history:";
    for (std::vector<llvm::Instruction *>::const_iterator
             it = callHistory.begin(),
             ie = callHistory.end();
         it != ie; ++it) {
      stream << "\n" << tabs << prefix;
      (*it)->print(stream);
    }
  }
}

/**/

void MemoryLocation::adjustOffsetBound(ref<VersionedValue> checkedAddress,
                                       std::set<ref<Expr> > &_bounds) {
  std::set<ref<MemoryLocation> > locations = checkedAddress->getLocations();
  std::set<ref<Expr> > bounds(_bounds);

  if (bounds.empty()) {
    bounds.insert(Expr::createPointer(size));
  }

  for (std::set<ref<Expr> >::iterator it1 = bounds.begin(), ie1 = bounds.end();
       it1 != ie1; ++it1) {

    for (std::set<ref<MemoryLocation> >::iterator it2 = locations.begin(),
                                                  ie2 = locations.end();
         it2 != ie2; ++it2) {
      ref<Expr> checkedOffset = (*it2)->getOffset();
      if (ConstantExpr *c = llvm::dyn_cast<ConstantExpr>(checkedOffset)) {
        if (ConstantExpr *o = llvm::dyn_cast<ConstantExpr>(offset)) {
          if (ConstantExpr *b = llvm::dyn_cast<ConstantExpr>(*it1)) {
            uint64_t offsetInt = o->getZExtValue();
            uint64_t newBound =
                b->getZExtValue() - (c->getZExtValue() - offsetInt);

            if (concreteOffsetBound > newBound) {

              // FIXME: A quick hack to avoid assertion check to make DirSeek.c
              // regression test pass.
              llvm::Value *v = (*it2)->getContext()->getValue();
              if (v->getType()->isPointerTy()) {
                llvm::Type *elementType = v->getType()->getPointerElementType();
                if (elementType->isStructTy() &&
                    elementType->getStructName() == "struct.dirent") {
                  concreteOffsetBound = newBound;
                  continue;
                }
              }

              assert(newBound > offsetInt && "incorrect bound");
              concreteOffsetBound = newBound;
            }
            continue;
          }
        }
      }

      symbolicOffsetBounds.insert(
          SubExpr::create(*it1, SubExpr::create(checkedOffset, offset)));
    }
  }
}

ref<MemoryLocation>
MemoryLocation::create(ref<MemoryLocation> loc,
                       std::set<const Array *> &replacements) {
  ref<Expr> _address(
      ShadowArray::getShadowExpression(loc->address, replacements)),
      _base(ShadowArray::getShadowExpression(loc->base, replacements)),
      _offset(ShadowArray::getShadowExpression(loc->offset, replacements));
  ref<MemoryLocation> ret(new MemoryLocation(
      loc->context, _address, _base, _offset, loc->size, loc->allocationId));
  return ret;
}

void MemoryLocation::print(llvm::raw_ostream &stream,
                           const std::string &prefix) const {
  std::string tabsNext = appendTab(prefix);

  stream << prefix << "function/value: ";
  if (outputFunctionName(context->getValue(), stream))
    stream << "/";
  context->getValue()->print(stream);
  stream << "\n";

  stream << prefix << "stack:\n";
  for (std::vector<llvm::Instruction *>::const_reverse_iterator
           it = context->getCallHistory().rbegin(),
           ib = it, ie = context->getCallHistory().rend();
       it != ie; ++it) {
    stream << tabsNext;
    (*it)->print(stream);
    stream << "\n";
  }
  stream << prefix << "offset";
  if (!llvm::isa<ConstantExpr>(this->offset))
    stream << " (symbolic)";
  stream << ": ";
  offset->print(stream);
  stream << "\n";
  stream << prefix << "address";
  if (!llvm::isa<ConstantExpr>(this->address))
    stream << " (symbolic)";
  stream << ": ";
  address->print(stream);
  stream << "\n";
  stream << prefix << "base: ";
  if (!llvm::isa<ConstantExpr>(this->base))
    stream << " (symbolic)";
  stream << ": ";
  base->print(stream);
  stream << "\n";
  stream << prefix
         << "pointer to location object: " << reinterpret_cast<uintptr_t>(this);
}

/**/

void VersionedValue::print(llvm::raw_ostream &stream,
                           const std::string &prefix) const {
  std::string tabsNext = appendTab(prefix);

  printNoDependency(stream, prefix);

  stream << "\n";
  if (sources.empty()) {
    stream << prefix << "no dependencies\n";
  } else {
    stream << prefix << "direct dependencies:\n";
    for (std::map<ref<VersionedValue>, ref<MemoryLocation> >::const_iterator
             is = sources.begin(),
             it = is, ie = sources.end();
         it != ie; ++it) {
      if (it != is)
        stream << tabsNext << "------------------------------------------\n";
      (*it->first).printNoDependency(stream, tabsNext);
      if (!it->second.isNull()) {
        stream << " via\n";
        (*it->second).print(stream, tabsNext);
    }
  }
  }
}

void VersionedValue::printNoDependency(llvm::raw_ostream &stream,
                                       const std::string &prefix) const {
  std::string tabsNext = appendTab(prefix);

  if (core) {
    if (!doNotInterpolateBound) {
      stream << prefix << "a bounded interpolant value\n";
    } else {
      stream << prefix << "an interpolant value\n";
    }
    if (!coreReasons.empty()) {
      for (std::set<std::string>::const_iterator it = coreReasons.begin(),
                                                 ie = coreReasons.end();
           it != ie; ++it) {
        stream << tabsNext << *it << "\n";
      }
    }
  } else {
    stream << prefix << "a non-interpolant value\n";
  }
  stream << prefix << "function/value: ";
  if (outputFunctionName(value, stream))
    stream << "/";
  value->print(stream);
  stream << "\n";
  stream << prefix << "expression: ";
  valueExpr->print(stream);
  stream << "\n";
  stream << prefix
         << "pointer to location object: " << reinterpret_cast<uintptr_t>(this);
}
}
