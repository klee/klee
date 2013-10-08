//===-- ModuleUtil.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/Support/ModuleUtil.h"
#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/DataStream.h"
#else
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Module.h"
#endif

#include "llvm/Linker.h"
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 8)
#include "llvm/Assembly/AsmAnnotationWriter.h"
#else
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#endif
#include "llvm/Support/CFG.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ValueTracking.h"
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
#include "llvm/System/Path.h"
#else
#include "llvm/Support/Path.h"
#endif

#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace llvm;
using namespace klee;

Module *klee::linkWithLibrary(Module *module, 
                              const std::string &libraryName) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
  SMDiagnostic err;
  std::string err_str;
  sys::Path libraryPath(libraryName);
  Module *new_mod = ParseIRFile(libraryPath.str(), err, 
module->getContext());

  if (Linker::LinkModules(module, new_mod, Linker::DestroySource, 
&err_str)) {
    assert(0 && "linked in library failed!");
  }

  return module;
#else
  Linker linker("klee", module, false);

  llvm::sys::Path libraryPath(libraryName);
  bool native = false;
    
  if (linker.LinkInFile(libraryPath, native)) {
    assert(0 && "linking in library failed!");
  }
    
  return linker.releaseModule();
#endif
}

Function *klee::getDirectCallTarget(CallSite cs) {
  Value *v = cs.getCalledValue();
  if (Function *f = dyn_cast<Function>(v)) {
    return f;
  } else if (llvm::ConstantExpr *ce = dyn_cast<llvm::ConstantExpr>(v)) {
    if (ce->getOpcode()==Instruction::BitCast)
      if (Function *f = dyn_cast<Function>(ce->getOperand(0)))
        return f;

    // NOTE: This assert may fire, it isn't necessarily a problem and
    // can be disabled, I just wanted to know when and if it happened.
    assert(0 && "FIXME: Unresolved direct target for a constant expression.");
  }
  
  return 0;
}

static bool valueIsOnlyCalled(const Value *v) {
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 8)
  for (Value::use_const_iterator it = v->use_begin(), ie = v->use_end();
       it != ie; ++it) {
#else
  for (Value::const_use_iterator it = v->use_begin(), ie = v->use_end();
       it != ie; ++it) {
#endif
    if (const Instruction *instr = dyn_cast<Instruction>(*it)) {
      if (instr->getOpcode()==0) continue; // XXX function numbering inst
      if (!isa<CallInst>(instr) && !isa<InvokeInst>(instr)) return false;
      
      // Make sure that the value is only the target of this call and
      // not an argument.
      for (unsigned i=1,e=instr->getNumOperands(); i!=e; ++i)
        if (instr->getOperand(i)==v)
          return false;
    } else if (const llvm::ConstantExpr *ce = 
               dyn_cast<llvm::ConstantExpr>(*it)) {
      if (ce->getOpcode()==Instruction::BitCast)
        if (valueIsOnlyCalled(ce))
          continue;
      return false;
    } else if (const GlobalAlias *ga = dyn_cast<GlobalAlias>(*it)) {
      // XXX what about v is bitcast of aliasee?
      if (v==ga->getAliasee() && !valueIsOnlyCalled(ga))
        return false;
    } else {
      return false;
    }
  }

  return true;
}

bool klee::functionEscapes(const Function *f) {
  return !valueIsOnlyCalled(f);
}
