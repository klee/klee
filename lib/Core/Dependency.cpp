//===-- Dependency.cpp - Field-insensitive dependency -----------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the dependency analysis to
/// compute the locations upon which the unsatisfiability core depends,
/// which is used in computing the interpolant.
///
//===----------------------------------------------------------------------===//

#include "Dependency.h"

#include "klee/CommandLine.h"
#include "klee/Internal/Support/ErrorHandling.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>
#else
#include <llvm/Constants.h>
#include <llvm/Intrinsics.h>
#endif

using namespace klee;

namespace klee {

std::map<const Array *, const Array *> ShadowArray::shadowArray;

UpdateNode *
ShadowArray::getShadowUpdate(const UpdateNode *source,
                             std::set<const Array *> &replacements) {
  if (!source)
    return 0;

  return new UpdateNode(getShadowUpdate(source->next, replacements),
                        getShadowExpression(source->index, replacements),
                        getShadowExpression(source->value, replacements));
}

ref<Expr> ShadowArray::createBinaryOfSameKind(ref<Expr> originalExpr,
                                              ref<Expr> newLhs,
                                              ref<Expr> newRhs) {
  std::vector<Expr::CreateArg> exprs;
  Expr::CreateArg arg1(newLhs);
  Expr::CreateArg arg2(newRhs);
  exprs.push_back(arg1);
  exprs.push_back(arg2);
  return Expr::createFromKind(originalExpr->getKind(), exprs);
}

void ShadowArray::addShadowArrayMap(const Array *source, const Array *target) {
  shadowArray[source] = target;
}

ref<Expr>
ShadowArray::getShadowExpression(ref<Expr> expr,
                                 std::set<const Array *> &replacements) {
  ref<Expr> ret;

  switch (expr->getKind()) {
  case Expr::Read: {
    ReadExpr *readExpr = static_cast<ReadExpr *>(expr.get());
    const Array *replacementArray = shadowArray[readExpr->updates.root];

    if (std::find(replacements.begin(), replacements.end(), replacementArray) ==
        replacements.end()) {
      replacements.insert(replacementArray);
    }

    UpdateList newUpdates(
        replacementArray,
        getShadowUpdate(readExpr->updates.head, replacements));
    ret = ReadExpr::alloc(newUpdates,
                          getShadowExpression(readExpr->index, replacements));
    break;
  }
  case Expr::Constant: {
    ret = expr;
    break;
  }
  case Expr::Select: {
    ret = SelectExpr::alloc(getShadowExpression(expr->getKid(0), replacements),
                            getShadowExpression(expr->getKid(1), replacements),
                            getShadowExpression(expr->getKid(2), replacements));
    break;
  }
  case Expr::Extract: {
    ExtractExpr *extractExpr = static_cast<ExtractExpr *>(expr.get());
    ret = ExtractExpr::alloc(getShadowExpression(expr->getKid(0), replacements),
                             extractExpr->offset, extractExpr->width);
    break;
  }
  case Expr::ZExt: {
    CastExpr *castExpr = static_cast<CastExpr *>(expr.get());
    ret = ZExtExpr::alloc(getShadowExpression(expr->getKid(0), replacements),
                          castExpr->getWidth());
    break;
  }
  case Expr::SExt: {
    CastExpr *castExpr = static_cast<CastExpr *>(expr.get());
    ret = SExtExpr::alloc(getShadowExpression(expr->getKid(0), replacements),
                          castExpr->getWidth());
    break;
  }
  case Expr::Concat:
  case Expr::Add:
  case Expr::Sub:
  case Expr::Mul:
  case Expr::UDiv:
  case Expr::SDiv:
  case Expr::URem:
  case Expr::SRem:
  case Expr::Not:
  case Expr::And:
  case Expr::Or:
  case Expr::Xor:
  case Expr::Shl:
  case Expr::LShr:
  case Expr::AShr:
  case Expr::Eq:
  case Expr::Ne:
  case Expr::Ult:
  case Expr::Ule:
  case Expr::Ugt:
  case Expr::Uge:
  case Expr::Slt:
  case Expr::Sle:
  case Expr::Sgt:
  case Expr::Sge: {
    ret = createBinaryOfSameKind(
        expr, getShadowExpression(expr->getKid(0), replacements),
        getShadowExpression(expr->getKid(1), replacements));
    break;
  }
  case Expr::NotOptimized: {
    ret = NotOptimizedExpr::create(getShadowExpression(expr->getKid(0), replacements));
    break;
  }
  default:
    assert(!"unhandled Expr type");
  }

  return ret;
}

/**/

void MemoryLocation::print(llvm::raw_ostream &stream) const {
  stream << "A";
  if (!llvm::isa<ConstantExpr>(this->address.get()))
    stream << "(symbolic)";
  if (core)
    stream << "(I)";
  stream << "[";
  loc->print(stream);
  stream << ":";
  address->print(stream);
  stream << "]#" << reinterpret_cast<uintptr_t>(this);
}

/**/

void VersionedValue::print(llvm::raw_ostream &stream) const {
  stream << "V";
  if (core)
    stream << "(I)";
  stream << "[";
  value->print(stream);
  stream << ":";
  valueExpr->print(stream);
  stream << "]#" << reinterpret_cast<uintptr_t>(this);
  ;
}

/**/

VersionedValue *Dependency::registerNewVersionedValue(llvm::Value *value,
                                                      VersionedValue *vvalue) {
  if (valuesMap.find(value) != valuesMap.end()) {
    valuesMap[value].push_back(vvalue);
  } else {
    std::vector<VersionedValue *> newValueList;
    newValueList.push_back(vvalue);
    valuesMap[value] = newValueList;
  }
  return vvalue;
}

std::vector<ref<MemoryLocation> >
Dependency::getAllVersionedLocations(bool coreOnly) const {
  std::vector<ref<MemoryLocation> > allLoc;

  if (coreOnly)
    std::copy(coreLocations.begin(), coreLocations.end(),
              std::back_inserter(allLoc));

  if (parent) {
    std::vector<ref<MemoryLocation> > parentVersionedLocations =
        parent->getAllVersionedLocations(coreOnly);
    allLoc.insert(allLoc.begin(), parentVersionedLocations.begin(),
                  parentVersionedLocations.end());
  }
  return allLoc;
}

std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
Dependency::getStoredExpressions(std::set<const Array *> &replacements,
                                 bool coreOnly) {
  std::vector<ref<MemoryLocation> > allLoc = getAllVersionedLocations(coreOnly);
  ConcreteStore concreteStore;
  SymbolicStore symbolicStore;

  for (std::vector<ref<MemoryLocation> >::iterator locIter = allLoc.begin(),
                                                   locIterEnd = allLoc.end();
       locIter != locIterEnd; ++locIter) {
    VersionedValue *storedValue = concreteStoresMap[*locIter];

    if (storedValue) {
      if ((*locIter)->hasConstantAddress()) {
        if (!coreOnly) {
          ref<Expr> expr = storedValue->getExpression();
          llvm::Value *site = (*locIter)->getValue();
          uint64_t uintAddress = (*locIter)->getUIntAddress();
          ref<Expr> address = (*locIter)->getAddress();
          concreteStore[site][uintAddress] = AddressValuePair(address, expr);
        } else if (storedValue->isCore()) {
          ref<Expr> expr = storedValue->getExpression();
          llvm::Value *base = (*locIter)->getValue();
          uint64_t uintAddress = (*locIter)->getUIntAddress();
          ref<Expr> address = (*locIter)->getAddress();
#ifdef ENABLE_Z3
          if (!NoExistential) {
            concreteStore[base][uintAddress] = AddressValuePair(
                ShadowArray::getShadowExpression(address, replacements),
                ShadowArray::getShadowExpression(expr, replacements));
	  } else
#endif
            concreteStore[base][uintAddress] = AddressValuePair(address, expr);
        }
      } else {
        ref<Expr> address = (*locIter)->getAddress();
        if (!coreOnly) {
          ref<Expr> expr = storedValue->getExpression();
          llvm::Value *base = (*locIter)->getValue();
          symbolicStore[base].push_back(AddressValuePair(address, expr));
        } else if (storedValue->isCore()) {
          ref<Expr> expr = storedValue->getExpression();
          llvm::Value *base = storedValue->getValue();
#ifdef ENABLE_Z3
          if (!NoExistential) {
            symbolicStore[base].push_back(AddressValuePair(
                ShadowArray::getShadowExpression(address, replacements),
                ShadowArray::getShadowExpression(expr, replacements)));
          } else
#endif
            symbolicStore[base].push_back(AddressValuePair(address, expr));
        }
      }
    }
  }

  return std::pair<ConcreteStore, SymbolicStore>(concreteStore, symbolicStore);
}

VersionedValue *Dependency::getLatestValue(llvm::Value *value,
                                           ref<Expr> valueExpr) {
  assert(value && "value cannot be null");
  if (llvm::isa<llvm::ConstantExpr>(value)) {
    llvm::Instruction *asInstruction =
        llvm::dyn_cast<llvm::ConstantExpr>(value)->getAsInstruction();
    if (llvm::isa<llvm::GetElementPtrInst>(asInstruction)) {
      return getNewPointerValue(value, valueExpr);
    }
  }

  // A global value is a constant: Its value is constant throughout execution,
  // but
  // indeterministic. In case this was a non-global-value (normal) constant, we
  // immediately return with a versioned value, as dependencies are not
  // important. However, the dependencies of global values should be searched
  // for in the ancestors (later) as they need to be consistent in an execution.
  if (llvm::isa<llvm::Constant>(value) && !llvm::isa<llvm::GlobalValue>(value))
    return getNewVersionedValue(value, valueExpr);

  if (valuesMap.find(value) != valuesMap.end()) {
    return valuesMap[value].back();
  }

  VersionedValue *ret = 0;
  if (parent)
    ret = parent->getLatestValue(value, valueExpr);

  if (!ret && llvm::isa<llvm::GlobalValue>(value)) {
    // We could not find the global value: we register it anew.
    ret = value->getType()->isPointerTy()
              ? getNewPointerValue(value, valueExpr)
              : getNewVersionedValue(value, valueExpr);
  }

  return ret;
}

VersionedValue *Dependency::getLatestValueNoConstantCheck(llvm::Value *value) {
  assert(value && "value cannot be null");

  if (valuesMap.find(value) != valuesMap.end()) {
    return valuesMap[value].back();
  }

  if (parent)
    return parent->getLatestValueNoConstantCheck(value);

  return 0;
}

void Dependency::updateStore(ref<MemoryLocation> loc, VersionedValue *value) {
  if (loc->hasConstantAddress())
    concreteStoresMap[loc] = value;
  else
    symbolicStoresMap[loc] = value;
}

void Dependency::addDependency(VersionedValue *source, VersionedValue *target) {
  std::set<ref<MemoryLocation> > locations = source->getLocations();

  for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    target->addLocation(*it);
  }
  addDependencyViaLocation(source, target, 0);
}

void Dependency::addDependencyWithOffset(VersionedValue *source,
                                         VersionedValue *target,
                                         ref<Expr> offset) {
  std::set<ref<MemoryLocation> > locations = source->getLocations();
  for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                ie = locations.end();
       it != ie; ++it) {
    ref<Expr> expr(target->getExpression());
    target->addLocation(MemoryLocation::create(*it, expr, offset));
  }
  addDependencyViaLocation(source, target, 0);
}

void Dependency::addDependencyViaLocation(VersionedValue *source,
                                          VersionedValue *target,
                                          ref<MemoryLocation> via) {
  if (flowsToMap.find(target) != flowsToMap.end()) {
    flowsToMap[target].insert(
        std::make_pair<VersionedValue *, ref<MemoryLocation> >(source, via));
  } else {
    std::map<VersionedValue *, ref<MemoryLocation> > newMap;
    newMap.insert(
        std::make_pair<VersionedValue *, ref<MemoryLocation> >(source, via));
    flowsToMap[target] = newMap;
  }
}

std::vector<VersionedValue *>
Dependency::directFlowSources(VersionedValue *target) const {
  std::vector<VersionedValue *> ret;
  if (flowsToMap.find(target) != flowsToMap.end()) {
    std::map<VersionedValue *, ref<MemoryLocation> > sources =
        flowsToMap.find(target)->second;
    for (std::map<VersionedValue *, ref<MemoryLocation> >::iterator it =
             sources.begin();
         it != sources.end(); ++it) {
      if (!it->first->isCore()) {
        ret.push_back(it->first);
      }
    }
  }

  if (parent) {
    std::vector<VersionedValue *> ancestralSources =
        parent->directFlowSources(target);
    ret.insert(ret.begin(), ancestralSources.begin(), ancestralSources.end());
  }
  return ret;
}

void Dependency::markFlow(VersionedValue *target) const {
  target->setAsCore();
  std::vector<VersionedValue *> stepSources = directFlowSources(target);
  for (std::vector<VersionedValue *>::iterator it = stepSources.begin(),
                                               ie = stepSources.end();
       it != ie; ++it) {
    markFlow(*it);
  }
}

std::vector<VersionedValue *>
Dependency::populateArgumentValuesList(llvm::CallInst *site,
                                       std::vector<ref<Expr> > &arguments) {
  unsigned numArgs = site->getCalledFunction()->arg_size();
  std::vector<VersionedValue *> argumentValuesList;
  for (unsigned i = numArgs; i > 0;) {
    llvm::Value *argOperand = site->getArgOperand(--i);
    VersionedValue *latestValue = getLatestValue(argOperand, arguments.at(i));

    if (latestValue)
      argumentValuesList.push_back(latestValue);
    else {
      // This is for the case when latestValue was NULL, which means there is
      // no source dependency information for this node, e.g., a constant.
      argumentValuesList.push_back(
          new VersionedValue(argOperand, arguments[i]));
    }
  }
  return argumentValuesList;
}

Dependency::Dependency(Dependency *parent)
    : parent(parent), concreteStoresMap(parent->concreteStoresMap),
      symbolicStoresMap(parent->symbolicStoresMap) {}

Dependency::~Dependency() {
  // Delete the locally-constructed relations
  concreteStoresMap.clear();
  symbolicStoresMap.clear();

  // Delete flowsToMap
  for (std::map<VersionedValue *,
                std::map<VersionedValue *, ref<MemoryLocation> > >::iterator
           it = flowsToMap.begin(),
           ie = flowsToMap.end();
       it != ie; ++it) {
    it->second.clear();
  }
  flowsToMap.clear();

  // Delete valuesMap
  for (std::map<llvm::Value *, std::vector<VersionedValue *> >::iterator
           it = valuesMap.begin(),
           ie = valuesMap.end();
       it != ie; ++it) {
    it->second.clear();
  }
  valuesMap.clear();
}

Dependency *Dependency::cdr() const { return parent; }

void Dependency::execute(llvm::Instruction *instr,
                         std::vector<ref<Expr> > &args,
                         bool symbolicExecutionError) {
  // The basic design principle that we need to be careful here
  // is that we should not store quadratic-sized structures in
  // the database of computed relations, e.g., not storing the
  // result of traversals of the graph. We keep the
  // quadratic blow up for only when querying the database.

  if (llvm::isa<llvm::CallInst>(instr)) {
    llvm::CallInst *callInst = llvm::dyn_cast<llvm::CallInst>(instr);
    llvm::Function *f = callInst->getCalledFunction();

    if (!f) {
	// Handles the case when the callee is wrapped within another expression
	llvm::ConstantExpr *calledValue = llvm::dyn_cast<llvm::ConstantExpr>(callInst->getCalledValue());
	if (calledValue && calledValue->getOperand(0)) {
	    f = llvm::dyn_cast<llvm::Function>(calledValue->getOperand(0));
	}
    }

    if (f && f->getIntrinsicID() == llvm::Intrinsic::not_intrinsic) {
      llvm::StringRef calleeName = f->getName();
      // FIXME: We need a more precise way to determine invoked method
      // rather than just using the name.
      std::string getValuePrefix("klee_get_value");

      if ((calleeName.equals("getpagesize") && args.size() == 1) ||
          (calleeName.equals("ioctl") && args.size() == 4) ||
          (calleeName.equals("__ctype_b_loc") && args.size() == 1) ||
          (calleeName.equals("__ctype_b_locargs") && args.size() == 1) ||
          calleeName.equals("puts") || calleeName.equals("fflush") ||
          calleeName.equals("_Znwm") || calleeName.equals("_Znam") ||
          calleeName.equals("strcmp") || calleeName.equals("strncmp") ||
          (calleeName.equals("__errno_location") && args.size() == 1) ||
          (calleeName.equals("geteuid") && args.size() == 1)) {
        getNewVersionedValue(instr, args.at(0));
      } else if (calleeName.equals("_ZNSi5seekgElSt12_Ios_Seekdir") &&
                 args.size() == 4) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i < 3; ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i), args.at(i + 1));
          if (arg)
            addDependency(arg, returnValue);
        }
      } else if (calleeName.equals(
                     "_ZNSt13basic_fstreamIcSt11char_traitsIcEE7is_openEv") &&
                 args.size() == 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg = getLatestValue(instr->getOperand(0), args.at(1));
        if (arg)
          addDependency(arg, returnValue);
      } else if (calleeName.equals("_ZNSi5tellgEv") && args.size() == 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg = getLatestValue(instr->getOperand(0), args.at(1));
        if (arg)
          addDependency(arg, returnValue);
      } else if ((calleeName.equals("powl") && args.size() == 3) ||
                 (calleeName.equals("gettimeofday") && args.size() == 3)) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i), args.at(i + 1));
          if (arg)
            addDependency(arg, returnValue);
        }
      } else if (calleeName.equals("malloc") && args.size() == 1) {
        // malloc is an location-type instruction: its single argument is the
        // return address.
        getNewPointerValue(instr, args.at(0));
      } else if (calleeName.equals("realloc") && args.size() == 1) {
        // realloc is an location-type instruction: its single argument is the
        // return address.
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg = getLatestValue(instr->getOperand(0), args.at(0));
        if (arg)
          addDependency(arg, returnValue);
      } else if (calleeName.equals("calloc") && args.size() == 1) {
        // calloc is a location-type instruction: its single argument is the
        // return address.
        getNewPointerValue(instr, args.at(0));
      } else if (calleeName.equals("syscall") && args.size() >= 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i + 1 < args.size(); ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i), args.at(i + 1));
          if (arg)
            addDependency(arg, returnValue);
        }
      } else if (std::mismatch(getValuePrefix.begin(), getValuePrefix.end(),
                               calleeName.begin()).first ==
                     getValuePrefix.end() &&
                 args.size() == 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg = getLatestValue(instr->getOperand(0), args.at(1));
        if (arg)
          addDependency(arg, returnValue);
      } else if (calleeName.equals("getenv") && args.size() == 2) {
        getNewPointerValue(instr, args.at(0));
      } else if (calleeName.equals("printf") && args.size() >= 2) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *formatArg =
            getLatestValue(instr->getOperand(0), args.at(1));
        if (formatArg)
          addDependency(formatArg, returnValue);
        for (unsigned i = 2, argsNum = args.size(); i < argsNum; ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i - 1), args.at(i));
          if (arg)
            addDependency(arg, returnValue);
        }
      } else if (calleeName.equals("vprintf") && args.size() == 3) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        VersionedValue *arg0 = getLatestValue(instr->getOperand(0), args.at(1));
        VersionedValue *arg1 = getLatestValue(instr->getOperand(1), args.at(2));
        if (arg0)
          addDependency(arg0, returnValue);
        if (arg1)
          addDependency(arg1, returnValue);
      } else if (((calleeName.equals("fchmodat") && args.size() == 5)) ||
                 (calleeName.equals("fchownat") && args.size() == 6)) {
        VersionedValue *returnValue = getNewVersionedValue(instr, args.at(0));
        for (unsigned i = 0; i < 2; ++i) {
          VersionedValue *arg =
              getLatestValue(instr->getOperand(i), args.at(i + 1));
          if (arg)
            addDependency(arg, returnValue);
        }
      } else {
        // Default external function handler: We ignore functions that return
        // void, and we DO NOT build dependency of return value to the
        // arguments.
        if (!instr->getType()->isVoidTy()) {
          assert(args.size() && "non-void call missing return expression");
          klee_warning("using default handler for external function %s",
                       calleeName.str().c_str());
          getNewVersionedValue(instr, args.at(0));
        }
      }
    }
    return;
  }

  switch (args.size()) {
  case 0: {
    switch (instr->getOpcode()) {
    case llvm::Instruction::Br: {
      llvm::BranchInst *binst = llvm::dyn_cast<llvm::BranchInst>(instr);
      if (binst && binst->isConditional()) {
        markAllValues(binst->getCondition());
      }
      break;
    }
    default:
      break;
    }
    return;
  }
  case 1: {
    ref<Expr> argExpr = args.at(0);

    switch (instr->getOpcode()) {
    case llvm::Instruction::Alloca: {
      getNewPointerValue(instr, argExpr);
      break;
    }
    case llvm::Instruction::Trunc:
    case llvm::Instruction::ZExt:
    case llvm::Instruction::SExt:
    case llvm::Instruction::IntToPtr:
    case llvm::Instruction::PtrToInt:
    case llvm::Instruction::BitCast:
    case llvm::Instruction::FPTrunc:
    case llvm::Instruction::FPExt:
    case llvm::Instruction::FPToUI:
    case llvm::Instruction::FPToSI:
    case llvm::Instruction::UIToFP:
    case llvm::Instruction::SIToFP:
    case llvm::Instruction::ExtractValue: {
      VersionedValue *val = getLatestValue(instr->getOperand(0), argExpr);

      if (val) {
        addDependency(val, getNewVersionedValue(instr, argExpr));
      } else if (!llvm::isa<llvm::Constant>(instr->getOperand(0)))
          // Constants would kill dependencies, the remaining is for
          // cases that may actually require dependencies.
      {
        if (instr->getOperand(0)->getType()->isPointerTy()) {
          VersionedValue *addressValue =
              getNewPointerValue(instr->getOperand(0), argExpr);
          VersionedValue *returnValue = getNewVersionedValue(instr, argExpr);
          addDependency(addressValue, returnValue);
        } else if (llvm::isa<llvm::Argument>(instr->getOperand(0)) ||
                   llvm::isa<llvm::CallInst>(instr->getOperand(0)) ||
                   symbolicExecutionError) {
          VersionedValue *arg =
              getNewVersionedValue(instr->getOperand(0), argExpr);
          VersionedValue *returnValue = getNewVersionedValue(instr, argExpr);
          addDependency(arg, returnValue);
        } else {
          assert(!"operand not found");
        }
      }
      break;
    }
    default: { assert(!"unhandled unary instruction"); }
    }
    return;
  }
  case 2: {
    ref<Expr> valueExpr = args.at(0);
    ref<Expr> address = args.at(1);

    switch (instr->getOpcode()) {
    case llvm::Instruction::Load: {
      VersionedValue *addressValue =
          getLatestValue(instr->getOperand(0), address);
      VersionedValue *loadedValue = getNewVersionedValue(instr, valueExpr);

      if (addressValue) {
        std::set<ref<MemoryLocation> > locations = addressValue->getLocations();
        if (locations.empty()) {
          ref<MemoryLocation> loc =
              MemoryLocation::create(instr->getOperand(0), address);
          addressValue->addLocation(loc);
          updateStore(loc, loadedValue);
          break;
        } else if (locations.size() == 1) {
          ref<MemoryLocation> loc = *(locations.begin());
          if (Util::isMainArgument(loc->getValue())) {
            // The load corresponding to a load of the main function's argument
            // that was never allocated within this program.
            updateStore(loc, loadedValue);
            break;
          }
        }
      } else {
        addressValue = getNewPointerValue(instr->getOperand(0), address);
        if (llvm::isa<llvm::GlobalVariable>(instr->getOperand(0))) {
          // The value not found was a global variable, record it here.
          std::set<ref<MemoryLocation> > locations =
              addressValue->getLocations();
          updateStore(*(locations.begin()), loadedValue);
          break;
        }
      }

      std::set<ref<MemoryLocation> > locations = addressValue->getLocations();

      for (std::set<ref<MemoryLocation> >::iterator
               locIter = locations.begin(),
               locIterEnd = locations.end();
           locIter != locIterEnd; ++locIter) {

        VersionedValue *storedValue = concreteStoresMap[*locIter];

        if (!storedValue)
          // We could not find the stored value, create a new one.
          updateStore(*locIter, loadedValue);
        else
          addDependencyViaLocation(storedValue, loadedValue, *locIter);
      }

      break;
    }
    case llvm::Instruction::Store: {
      VersionedValue *storedValue =
          getLatestValue(instr->getOperand(0), valueExpr);
      VersionedValue *addressValue =
          getLatestValue(instr->getOperand(1), address);

      // If there was no dependency found, we should create
      // a new value
      if (!storedValue)
        storedValue = getNewVersionedValue(instr->getOperand(0), valueExpr);

      if (!addressValue) {
        addressValue = getNewPointerValue(instr->getOperand(1), address);
      } else if (addressValue->getLocations().size() == 0) {
        addressValue->addLocation(
            MemoryLocation::create(instr->getOperand(1), address));
      }

      std::set<ref<MemoryLocation> > locations = addressValue->getLocations();

      for (std::set<ref<MemoryLocation> >::iterator it = locations.begin(),
                                                    ie = locations.end();
           it != ie; ++it) {
        updateStore(*it, storedValue);
      }
      break;
    }
    default: { assert(!"unhandled binary instruction"); }
    }
    return;
  }
  case 3: {
    ref<Expr> result = args.at(0);
    ref<Expr> op1Expr = args.at(1);
    ref<Expr> op2Expr = args.at(2);

    switch (instr->getOpcode()) {
    case llvm::Instruction::Select: {

      VersionedValue *op1 = getLatestValue(instr->getOperand(1), op1Expr);
      VersionedValue *op2 = getLatestValue(instr->getOperand(2), op2Expr);
      VersionedValue *newValue = 0;
      if (op1) {
        newValue = getNewVersionedValue(instr, result);
        addDependency(op1, newValue);
      }
      if (op2) {
        if (newValue)
          addDependency(op2, newValue);
        else
          addDependency(op2, getNewVersionedValue(instr, result));
      }
      break;
    }

    case llvm::Instruction::Add:
    case llvm::Instruction::Sub:
    case llvm::Instruction::Mul:
    case llvm::Instruction::UDiv:
    case llvm::Instruction::SDiv:
    case llvm::Instruction::URem:
    case llvm::Instruction::SRem:
    case llvm::Instruction::And:
    case llvm::Instruction::Or:
    case llvm::Instruction::Xor:
    case llvm::Instruction::Shl:
    case llvm::Instruction::LShr:
    case llvm::Instruction::AShr:
    case llvm::Instruction::ICmp:
    case llvm::Instruction::FAdd:
    case llvm::Instruction::FSub:
    case llvm::Instruction::FMul:
    case llvm::Instruction::FDiv:
    case llvm::Instruction::FRem:
    case llvm::Instruction::FCmp:
    case llvm::Instruction::InsertValue: {
      VersionedValue *op1 = getLatestValue(instr->getOperand(0), op1Expr);
      VersionedValue *op2 = getLatestValue(instr->getOperand(1), op2Expr);

      if (!op1 &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(0)->getName().equals("start"))) {
        op1 = getNewVersionedValue(instr->getOperand(0), op1Expr);
      }
      if (!op2 &&
          (instr->getParent()->getParent()->getName().equals("klee_range") &&
           instr->getOperand(1)->getName().equals("end"))) {
        op2 = getNewVersionedValue(instr->getOperand(1), op2Expr);
      }

      VersionedValue *newValue = 0;
      if (op1) {
        newValue = getNewVersionedValue(instr, result);
        addDependency(op1, newValue);
      }
      if (op2) {
        if (newValue)
          addDependency(op2, newValue);
        else
          addDependency(op2, getNewVersionedValue(instr, result));
      }
      break;
    }
    case llvm::Instruction::GetElementPtr: {
      ref<Expr> resultAddress = args.at(0);
      ref<Expr> inputAddress = args.at(1);
      ref<Expr> inputOffset = args.at(2);

      VersionedValue *addressValue =
          getLatestValue(instr->getOperand(0), inputAddress);
      if (!addressValue) {
        addressValue = getNewPointerValue(instr->getOperand(0), inputAddress);
      } else if (addressValue->getLocations().size() == 0) {
        addressValue->addLocation(
            MemoryLocation::create(instr->getOperand(0), inputAddress));
      }

      addDependencyWithOffset(addressValue,
                              getNewVersionedValue(instr, resultAddress),
                              inputOffset);
      break;
    }
    default:
      assert(!"unhandled ternary instruction");
    }
    return;
  }
  default:
    break;
  }
  assert(!"unhandled instruction arguments number");
}

void Dependency::executePHI(llvm::Instruction *instr,
                            unsigned int incomingBlock, ref<Expr> valueExpr,
                            bool symbolicExecutionError) {
  llvm::PHINode *node = llvm::dyn_cast<llvm::PHINode>(instr);
  llvm::Value *llvmArgValue = node->getIncomingValue(incomingBlock);
  VersionedValue *val = getLatestValue(llvmArgValue, valueExpr);
  if (val) {
    addDependency(val, getNewVersionedValue(instr, valueExpr));
  } else if (llvm::isa<llvm::Constant>(llvmArgValue) ||
             llvm::isa<llvm::Argument>(llvmArgValue) ||
             symbolicExecutionError) {
    getNewVersionedValue(instr, valueExpr);
  } else {
    assert(!"operand not found");
  }
}

void Dependency::executeMemoryOperation(llvm::Instruction *instr,
                                        std::vector<ref<Expr> > &args,
                                        bool boundsCheck,
                                        bool symbolicExecutionError) {
  execute(instr, args, symbolicExecutionError);
  if (boundsCheck) {
    // The bounds check has been proven valid, we keep the dependency on the
    // address. Calling va_start within a variadic function also triggers memory
    // operation, but we ignored it here as this method is only called when load
    // / store instruction is processed.
    llvm::Value *addressOperand;
    switch (instr->getOpcode()) {
    case llvm::Instruction::Load: {
      addressOperand = instr->getOperand(0);
      break;
    }
    case llvm::Instruction::Store: {
      addressOperand = instr->getOperand(1);
      break;
    }
    default: {
      assert(!"unknown memory operation");
      break;
    }
    }
    markAllValues(addressOperand);
  }
}

void Dependency::bindCallArguments(llvm::Instruction *i,
                                   std::vector<ref<Expr> > &arguments) {
  llvm::CallInst *site = llvm::dyn_cast<llvm::CallInst>(i);

  if (!site)
    return;

  llvm::Function *callee = site->getCalledFunction();

  // Sometimes the callee information is missing, in which case
  // the calle is not to be symbolically tracked.
  if (!callee)
    return;

  argumentValuesList = populateArgumentValuesList(site, arguments);
  unsigned index = 0;
  for (llvm::Function::ArgumentListType::iterator
           it = callee->getArgumentList().begin(),
           ie = callee->getArgumentList().end();
       it != ie; ++it) {
    if (argumentValuesList.back()) {
      addDependency(
          argumentValuesList.back(),
          getNewVersionedValue(it, argumentValuesList.back()->getExpression()));
    }
    argumentValuesList.pop_back();
    ++index;
  }
}

void Dependency::bindReturnValue(llvm::CallInst *site, llvm::Instruction *i,
                                 ref<Expr> returnValue) {
  llvm::ReturnInst *retInst = llvm::dyn_cast<llvm::ReturnInst>(i);
  if (site && retInst &&
      retInst->getReturnValue() // For functions returning void
      ) {
    VersionedValue *value =
        getLatestValue(retInst->getReturnValue(), returnValue);
    if (value)
      addDependency(value, getNewVersionedValue(site, returnValue));
  }
}

void Dependency::markAllValues(VersionedValue *value) { markFlow(value); }

void Dependency::markAllValues(llvm::Value *val) {
  VersionedValue *value = getLatestValueNoConstantCheck(val);

  // Right now we simply ignore the __dso_handle values. They are due
  // to library / linking errors caused by missing options (-shared) in the
  // compilation involving shared library.
  if (!value) {
    if (llvm::ConstantExpr *cVal = llvm::dyn_cast<llvm::ConstantExpr>(val)) {
      for (unsigned i = 0; i < cVal->getNumOperands(); ++i) {
        if (cVal->getOperand(i)->getName().equals("__dso_handle")) {
          return;
        }
      }
    }

    if (llvm::isa<llvm::Constant>(val))
      return;

    assert(!"unknown value");
  }

  markFlow(value);
}

void Dependency::print(llvm::raw_ostream &stream) const {
  this->print(stream, 0);
}

void Dependency::print(llvm::raw_ostream &stream,
                       const unsigned paddingAmount) const {
  std::string tabs = makeTabs(paddingAmount);
  std::map<ref<MemoryLocation>, VersionedValue *>::const_iterator
  storesMapBegin = concreteStoresMap.begin();
  std::map<VersionedValue *,
           std::map<VersionedValue *, ref<MemoryLocation> > >::const_iterator
  flowsToMapBegin = flowsToMap.begin();

  stream << tabs << "STORAGE:";
  for (std::map<ref<MemoryLocation>, VersionedValue *>::const_iterator
           it = concreteStoresMap.begin(),
           ie = concreteStoresMap.end();
       it != ie; ++it) {
    if (it != storesMapBegin)
      stream << ",";
    stream << "[";
    (*it->first).print(stream);
    stream << ",";
    (*it->second).print(stream);
    stream << "]";
  }
  stream << "\n";
  stream << tabs << "FLOWDEPENDENCY:";
  for (std::map<
           VersionedValue *,
           std::map<VersionedValue *, ref<MemoryLocation> > >::const_iterator
           it = flowsToMap.begin(),
           ie = flowsToMap.end();
       it != ie; ++it) {
    if (it != flowsToMapBegin)
      stream << ",";
    std::map<VersionedValue *, ref<MemoryLocation> > sources = (*it).second;
    std::map<VersionedValue *, ref<MemoryLocation> >::iterator sourcesMapBegin =
        sources.begin();
    for (std::map<VersionedValue *, ref<MemoryLocation> >::iterator it2 =
             sources.begin();
         it2 != sources.end(); ++it2) {
      if (it2 != sourcesMapBegin)
        stream << ",";
      stream << "[";
      (*it->first).print(stream);
      stream << " <- ";
      (*it2->first).print(stream);
      stream << "]";
      if (it2->second != NULL) {
        stream << " via ";
        (*it2->second).print(stream);
      }
    }
  }

  if (parent) {
    stream << "\n" << tabs << "--------- Parent Dependencies ----------\n";
    parent->print(stream, paddingAmount);
  }
}

/**/

bool Dependency::Util::isMainArgument(llvm::Value *loc) {
  llvm::Argument *vArg = llvm::dyn_cast<llvm::Argument>(loc);

  // FIXME: We need a more precise way to detect main argument
  if (vArg && vArg->getParent() &&
      (vArg->getParent()->getName().equals("main") ||
       vArg->getParent()->getName().equals("__user_main"))) {
    return true;
  }
  return false;
}

/**/

std::string makeTabs(const unsigned paddingAmount) {
  std::string tabsString;
  for (unsigned i = 0; i < paddingAmount; ++i) {
    tabsString += appendTab(tabsString);
  }
  return tabsString;
}

std::string appendTab(const std::string &prefix) { return prefix + "        "; }
}
