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
// FIXME: This does not belong here.
#include "../Core/Common.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Module.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/DataStream.h"
#else
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Module.h"
#endif

#include "llvm/Linker.h"
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/Path.h"

#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace llvm;
using namespace klee;

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
/// Based on GetAllUndefinedSymbols() from LLVM3.2
///
/// GetAllUndefinedSymbols - calculates the set of undefined symbols that still
/// exist in an LLVM module. This is a bit tricky because there may be two
/// symbols with the same name but different LLVM types that will be resolved to
/// each other but aren't currently (thus we need to treat it as resolved).
///
/// Inputs:
///  M - The module in which to find undefined symbols.
///
/// Outputs:
///  UndefinedSymbols - A set of C++ strings containing the name of all
///                     undefined symbols.
///
static void
GetAllUndefinedSymbols(Module *M, std::set<std::string> &UndefinedSymbols) {
  static const std::string llvmIntrinsicPrefix="llvm.";
  std::set<std::string> DefinedSymbols;
  UndefinedSymbols.clear();
  DEBUG_WITH_TYPE("klee_linker", dbgs() << "*** Computing undefined symbols ***\n");

  for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I)
    if (I->hasName()) {
      if (I->isDeclaration())
        UndefinedSymbols.insert(I->getName());
      else if (!I->hasLocalLinkage()) {
        assert(!I->hasDLLImportLinkage()
               && "Found dllimported non-external symbol!");
        DefinedSymbols.insert(I->getName());
      }
    }

  for (Module::global_iterator I = M->global_begin(), E = M->global_end();
       I != E; ++I)
    if (I->hasName()) {
      if (I->isDeclaration())
        UndefinedSymbols.insert(I->getName());
      else if (!I->hasLocalLinkage()) {
        assert(!I->hasDLLImportLinkage()
               && "Found dllimported non-external symbol!");
        DefinedSymbols.insert(I->getName());
      }
    }

  for (Module::alias_iterator I = M->alias_begin(), E = M->alias_end();
       I != E; ++I)
    if (I->hasName())
      DefinedSymbols.insert(I->getName());

  // Prune out any defined symbols from the undefined symbols set
  // and other symbols we don't want to treat as an undefined symbol
  for (std::set<std::string>::iterator I = UndefinedSymbols.begin();
       I != UndefinedSymbols.end(); )
  {
    if (DefinedSymbols.count(*I))
    {
      UndefinedSymbols.erase(I++);  // This symbol really is defined!
      continue;
    }

    // Strip out llvm intrinsics
    if ( (I->size() >= llvmIntrinsicPrefix.size() ) &&
       (I->compare(0, llvmIntrinsicPrefix.size(), llvmIntrinsicPrefix) == 0) )
    {
      DEBUG_WITH_TYPE("klee_linker", dbgs() << "LLVM intrinsic " << *I <<
                      " has been removed from undefined symbols"<< "\n");
      UndefinedSymbols.erase(I++);
      continue;
    }

    // Symbol really is undefined
    DEBUG_WITH_TYPE("klee_linker", dbgs() << "Symbol " << *I << " is undefined.\n");
    ++I; // Keep this symbol in the undefined symbols list
  }

  // FIXME: Remove KLEE special functions
  // from the list of undefined symbols. HandlerInfo in SpecialFunctionHandler
  // needs to be moved so we can use it here.

  DEBUG_WITH_TYPE("klee_linker", dbgs() << "*** Finished computing undefined symbols ***\n");
}



/*! A helper function for klee::linkWithLibrary() that links in an archive of bitcode
 *  modules into a composite bitcode module
 *
 *  \param[in] archive Archive of bitcode modules
 *  \param[in,out] composite The archive to link the archive with
 *  \param[out] errorMessage Set to an error message if linking fails
 *
 *  \return True if linking succeeds otherwise false
 */
static bool linkBCA(object::Archive* archive, Module* composite, std::string& errorMessage)
{
  llvm::raw_string_ostream SS(errorMessage);
  std::vector<Module*> archiveModules;

  // Is this efficient? Could we use StringRef instead?
  std::set<std::string> undefinedSymbols;
  GetAllUndefinedSymbols(composite, undefinedSymbols);

  if (undefinedSymbols.size() == 0)
  {
    // Nothing to do
    DEBUG_WITH_TYPE("klee_linker", dbgs() << "No undefined symbols. Not linking anything in!\n");
    return true;
  }

  DEBUG_WITH_TYPE("klee_linker", dbgs() << "Loading modules\n");
  // Load all bitcode files in to memory so we can examine their symbols
  for (object::Archive::child_iterator AI = archive->begin_children(),
       AE = archive->end_children(); AI != AE; ++AI)
  {

    StringRef memberName;
    error_code ec = AI->getName(memberName);

    if ( ec == errc::success )
    {
      DEBUG_WITH_TYPE("klee_linker", dbgs() << "Loading archive member " << memberName << "\n");
    }
    else
    {
      errorMessage="Archive member does not have a name!\n";
      return false;
    }

    OwningPtr<object::Binary> child;
    ec = AI->getAsBinary(child);
    if (ec != object::object_error::success)
    {
      // If we can't open as a binary object file its hopefully a bitcode file

      OwningPtr<MemoryBuffer> buff; // Once this is destroyed will Module still be valid??
      Module *Result = 0;

      if (error_code ec = AI->getMemoryBuffer(buff))
      {
        SS << "Failed to get MemoryBuffer: " <<ec.message();
        SS.flush();
        return false;
      }

      if (buff)
      {
        // FIXME: Maybe load bitcode file lazily? Then if we need to link, materialise the module
        Result = ParseBitcodeFile(buff.get(), getGlobalContext(), &errorMessage);

        if(!Result)
        {
          SS << "Loading module failed : " << errorMessage << "\n";
          SS.flush();
          return false;
        }
        archiveModules.push_back(Result);
      }
      else
      {
        errorMessage="Buffer was NULL!";
        return false;
      }

    }
    else if (object::ObjectFile *o = dyn_cast<object::ObjectFile>(child.get()))
    {
      SS << "Object file " << o->getFileName().data() <<
            " in archive is not supported";
      SS.flush();
      return false;
    }
    else
    {
      SS << "Loading archive child with error "<< ec.message();
      SS.flush();
      return false;
    }

  }

  DEBUG_WITH_TYPE("klee_linker", dbgs() << "Loaded " << archiveModules.size() << " modules\n");


  std::set<std::string> previouslyUndefinedSymbols;

  // Walk through the modules looking for definitions of undefined symbols
  // if we find a match we should link that module in.
  unsigned int passCounter=0;
  do
  {
    unsigned int modulesLoadedOnPass=0;
    previouslyUndefinedSymbols = undefinedSymbols;

    for (std::vector<Module*>::iterator M = archiveModules.begin(), ME = archiveModules.end();
        M != ME ;)
    {
      // Look for the undefined symbols in the composite module
      for (std::set<std::string>::iterator S = undefinedSymbols.begin(), SE = undefinedSymbols.end();
         S != SE; ++S)
      {
        // FIXME: We aren't handling weak symbols here!
        // However the algorithm used in LLVM3.2 didn't seem to either
        // so maybe it doesn't matter?

        if ( GlobalValue* GV = dyn_cast_or_null<GlobalValue>((**M).getValueSymbolTable().lookup(*S)))
        {
          if (GV->isDeclaration()) continue; // Not a definition

          Module* moduleToLinkIn = *M;


          DEBUG_WITH_TYPE("klee_linker", dbgs() << "Found " << GV->getName() <<
              " in " << (**M).getModuleIdentifier() << "\n");


          if (Linker::LinkModules(composite, moduleToLinkIn, Linker::DestroySource, &errorMessage))
          {
            // Linking failed
            SS << "Linking archive module with composite failed:" << errorMessage;
            SS.flush();
            return false;
          }
          else
          {
            // Link succeed, now clean up
            modulesLoadedOnPass++;
            DEBUG_WITH_TYPE("klee_linker", dbgs() << "Linking succeeded.\n");

            // Note use of postfix operator here so that the iterator gets incremented
            // BEFORE we remove it from the vector so the iterator is not invalidated
            archiveModules.erase(M++);
            delete moduleToLinkIn;

            // We need to recompute the undefined symbols in the composite module
            // after linking
            GetAllUndefinedSymbols(composite, undefinedSymbols);

            break; // Look for symbols in next module
          }

        }
      }

      ++M; // Try next module
    }

    passCounter++;
    DEBUG_WITH_TYPE("klee_linker", dbgs() << "Completed " << passCounter <<
                " linker passes.\n" << modulesLoadedOnPass <<
                " modules loaded on the last pass\n" <<
                archiveModules.size() << " modules left.\n");
  } while (undefinedSymbols != previouslyUndefinedSymbols); // Iterate until we reach a fixed point

  // What's left in archiveModules we don't want to link in so free it
  for (std::vector<Module*>::iterator I = archiveModules.begin(), E = archiveModules.end();
      I != E; ++I)
  {
    delete (*I);
  }


  return true;

}
#endif


Module *klee::linkWithLibrary(Module *module, 
                              const std::string &libraryName) {
DEBUG_WITH_TYPE("klee_linker", dbgs() << "Linking file " << libraryName << "\n");
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
  if (!sys::fs::exists(libraryName)) {
    klee_error("Link with library %s failed. No such file.",
        libraryName.c_str());
  }

  OwningPtr<MemoryBuffer> Buffer;
  if (error_code ec = MemoryBuffer::getFile(libraryName,Buffer)) {
    klee_error("Link with library %s failed: %s", libraryName.c_str(),
        ec.message().c_str());
  }

  sys::fs::file_magic magic = sys::fs::identify_magic(Buffer->getBuffer());

  LLVMContext &Context = getGlobalContext();
  std::string ErrorMessage;

  if (magic == sys::fs::file_magic::bitcode) {
    Module *Result = 0;
    Result = ParseBitcodeFile(Buffer.get(), Context, &ErrorMessage);


    if (!Result || Linker::LinkModules(module, Result, Linker::DestroySource,
        &ErrorMessage))
      klee_error("Link with library %s failed: %s", libraryName.c_str(),
          ErrorMessage.c_str());

    delete Result;

  } else if (magic == sys::fs::file_magic::archive) {
    OwningPtr<object::Binary> arch;
    if (error_code ec = object::createBinary(Buffer.take(), arch))
      klee_error("Link with library %s failed: %s", libraryName.c_str(),
          ec.message().c_str());

    if (object::Archive *a = dyn_cast<object::Archive>(arch.get())) {
      // Handle in helper
      if (!linkBCA(a, module, ErrorMessage))
        klee_error("Link with library %s failed: %s", libraryName.c_str(),
            ErrorMessage.c_str());
    }
    else {
    	klee_error("Link with library %s failed: Cast to archive failed", libraryName.c_str());
    }

  } else if (magic.is_object()) {
    OwningPtr<object::Binary> obj;
    if (object::ObjectFile *o = dyn_cast<object::ObjectFile>(obj.get())) {
      klee_warning("Link with library: Object file %s in archive %s found. "
          "Currently not supported.",
          o->getFileName().data(), libraryName.c_str());
    }
  } else {
    klee_error("Link with library %s failed: Unrecognized file type.",
        libraryName.c_str());
  }

  return module;
#else
  Linker linker("klee", module, false);

  llvm::sys::Path libraryPath(libraryName);
  bool native = false;
    
  if (linker.LinkInFile(libraryPath, native)) {
    klee_error("Linking library %s failed", libraryName.c_str());
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
  for (Value::const_use_iterator it = v->use_begin(), ie = v->use_end();
       it != ie; ++it) {
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
