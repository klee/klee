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
#include "klee/Internal/Support/Debug.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "../Core/SpecialFunctionHandler.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 4)
#include "llvm/IR/LLVMContext.h"
#endif

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Module.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/DataStream.h"
#else
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Module.h"
#endif

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
#include "llvm/Linker.h"
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#else
#include "llvm/Linker/Linker.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#endif

#if LLVM_VERSION_CODE <= LLVM_VERSION(2, 9)
// for llvm::error_code
#include "llvm/Support/system_error.h"
#endif

#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Path.h"

#include <map>
#include <set>
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
  KLEE_DEBUG_WITH_TYPE("klee_linker",
                       dbgs() << "*** Computing undefined symbols ***\n");

  for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I)
    if (I->hasName()) {
      if (I->isDeclaration())
        UndefinedSymbols.insert(I->getName());
      else if (!I->hasLocalLinkage()) {
#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
            assert(!I->hasDLLImportLinkage() && "Found dllimported non-external symbol!");
#else
            assert(!I->hasDLLImportStorageClass() && "Found dllimported non-external symbol!");
#endif
        DefinedSymbols.insert(I->getName());
      }
    }

  for (Module::global_iterator I = M->global_begin(), E = M->global_end();
       I != E; ++I)
    if (I->hasName()) {
      if (I->isDeclaration())
        UndefinedSymbols.insert(I->getName());
      else if (!I->hasLocalLinkage()) {
#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
            assert(!I->hasDLLImportLinkage() && "Found dllimported non-external symbol!");
#else
            assert(!I->hasDLLImportStorageClass() && "Found dllimported non-external symbol!");
#endif
        DefinedSymbols.insert(I->getName());
      }
    }

  for (Module::alias_iterator I = M->alias_begin(), E = M->alias_end();
       I != E; ++I)
    if (I->hasName())
      DefinedSymbols.insert(I->getName());


  // Prune out any defined symbols from the undefined symbols set
  // and other symbols we don't want to treat as an undefined symbol
  std::vector<std::string> SymbolsToRemove;
  for (std::set<std::string>::iterator I = UndefinedSymbols.begin();
       I != UndefinedSymbols.end(); ++I )
  {
    if (DefinedSymbols.count(*I))
    {
      SymbolsToRemove.push_back(*I);
      continue;
    }

    // Strip out llvm intrinsics
    if ( (I->size() >= llvmIntrinsicPrefix.size() ) &&
       (I->compare(0, llvmIntrinsicPrefix.size(), llvmIntrinsicPrefix) == 0) )
    {
      KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "LLVM intrinsic " << *I <<
                      " has will be removed from undefined symbols"<< "\n");
      SymbolsToRemove.push_back(*I);
      continue;
    }

    // Symbol really is undefined
    KLEE_DEBUG_WITH_TYPE("klee_linker",
                         dbgs() << "Symbol " << *I << " is undefined.\n");
  }

  // Remove KLEE intrinsics from set of undefined symbols
  for (SpecialFunctionHandler::const_iterator sf = SpecialFunctionHandler::begin(),
       se = SpecialFunctionHandler::end(); sf != se; ++sf)
  {
    if (UndefinedSymbols.find(sf->name) == UndefinedSymbols.end())
      continue;

    SymbolsToRemove.push_back(sf->name);
    KLEE_DEBUG_WITH_TYPE("klee_linker",
                         dbgs() << "KLEE intrinsic " << sf->name <<
                         " has will be removed from undefined symbols"<< "\n");
  }

  // Now remove the symbols from undefined set.
  for (size_t i = 0, j = SymbolsToRemove.size(); i < j; ++i )
    UndefinedSymbols.erase(SymbolsToRemove[i]);

  KLEE_DEBUG_WITH_TYPE("klee_linker",
                       dbgs() << "*** Finished computing undefined symbols ***\n");
}


/*!  A helper function for linkBCA() which cleans up
 *   memory allocated by that function.
 */
static void CleanUpLinkBCA(std::vector<Module*> &archiveModules)
{
  for (std::vector<Module*>::iterator I = archiveModules.begin(), E = archiveModules.end();
      I != E; ++I)
  {
    delete (*I);
  }
}

/*! A helper function for klee::linkWithLibrary() that links in an archive of bitcode
 *  modules into a composite bitcode module
 *
 *  \param[in] archive Archive of bitcode modules
 *  \param[in,out] composite The bitcode module to link against the archive
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
    KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "No undefined symbols. Not linking anything in!\n");
    return true;
  }

  KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "Loading modules\n");
  // Load all bitcode files in to memory so we can examine their symbols
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
  for (object::Archive::child_iterator AI = archive->child_begin(),
       AE = archive->child_end(); AI != AE; ++AI)
#else
  for (object::Archive::child_iterator AI = archive->begin_children(),
       AE = archive->end_children(); AI != AE; ++AI)
#endif
  {

    StringRef memberName;
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
    ErrorOr<StringRef> memberNameErr = AI->getName();
    std::error_code ec = memberNameErr.getError();
    if (!ec) {
      memberName = memberNameErr.get();
#else
    error_code ec = AI->getName(memberName);

    if ( ec == errc::success )
    {
#endif
      KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "Loading archive member " << memberName << "\n");
    }
    else
    {
      errorMessage="Archive member does not have a name!\n";
      return false;
    }

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
    ErrorOr<std::unique_ptr<llvm::object::Binary> > child = AI->getAsBinary();
    ec = child.getError();
#else
    OwningPtr<object::Binary> child;
    ec = AI->getAsBinary(child);
#endif
    if (ec) {
      // If we can't open as a binary object file its hopefully a bitcode file
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
      ErrorOr<MemoryBufferRef> buff = AI->getMemoryBufferRef();
      ec = buff.getError();
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
      ErrorOr<std::unique_ptr<MemoryBuffer> > buffErr = AI->getMemoryBuffer();
      std::unique_ptr<MemoryBuffer> buff = nullptr;
      ec = buffErr.getError();
      if (!ec)
        buff = std::move(buffErr.get());
#else
      OwningPtr<MemoryBuffer> buff; // Once this is destroyed will Module still be valid??
      ec = AI->getMemoryBuffer(buff);
#endif
      if (ec) {
        SS << "Failed to get MemoryBuffer: " <<ec.message();
        SS.flush();
        return false;
      }

      if (buff)
      {
        Module *Result = 0;
        // FIXME: Maybe load bitcode file lazily? Then if we need to link, materialise the module
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
        ErrorOr<Module *> resultErr = parseBitcodeFile(buff.get(),
	    composite->getContext());
        ec = resultErr.getError();
        if (ec)
          errorMessage = ec.message();
        else
          Result = resultErr.get();
#else
        Result = ParseBitcodeFile(buff.get(), composite->getContext(),
	    &errorMessage);
#endif
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
    else if (child.get()->isObject())
    {
      SS << "Object file " << child.get()->getFileName().data() <<
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

  KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "Loaded " << archiveModules.size() << " modules\n");


  std::set<std::string> previouslyUndefinedSymbols;

  // Walk through the modules looking for definitions of undefined symbols
  // if we find a match we should link that module in.
  unsigned int passCounter=0;
  do
  {
    unsigned int modulesLoadedOnPass=0;
    previouslyUndefinedSymbols = undefinedSymbols;

    for (size_t i = 0, j = archiveModules.size(); i < j; ++i)
    {
      // skip empty archives
      if (archiveModules[i] == 0)
        continue;
      Module * M = archiveModules[i];
      // Look for the undefined symbols in the composite module
      for (std::set<std::string>::iterator S = undefinedSymbols.begin(), SE = undefinedSymbols.end();
         S != SE; ++S)
      {

        // FIXME: We aren't handling weak symbols here!
        // However the algorithm used in LLVM3.2 didn't seem to either
        // so maybe it doesn't matter?

        if ( GlobalValue* GV = dyn_cast_or_null<GlobalValue>(M->getValueSymbolTable().lookup(*S)))
        {
          if (GV->isDeclaration()) continue; // Not a definition

          KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "Found " << GV->getName() <<
              " in " << M->getModuleIdentifier() << "\n");

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
          if (Linker::LinkModules(composite, M))
#else
          if (Linker::LinkModules(composite, M, Linker::DestroySource, &errorMessage))
#endif
          {
            // Linking failed
            SS << "Linking archive module with composite failed:" << errorMessage;
            SS.flush();
            CleanUpLinkBCA(archiveModules);
            return false;
          }
          else
          {
            // Link succeed, now clean up
            modulesLoadedOnPass++;
            KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "Linking succeeded.\n");

            delete M;
            archiveModules[i] = 0;

            // We need to recompute the undefined symbols in the composite module
            // after linking
            GetAllUndefinedSymbols(composite, undefinedSymbols);

            break; // Look for symbols in next module
          }

        }
      }
    }

    passCounter++;
    KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "Completed " << passCounter <<
                " linker passes.\n" << modulesLoadedOnPass <<
                " modules loaded on the last pass\n");
  } while (undefinedSymbols != previouslyUndefinedSymbols); // Iterate until we reach a fixed point


  // What's left in archiveModules we don't want to link in so free it
  CleanUpLinkBCA(archiveModules);

  return true;

}
#endif


Module *klee::linkWithLibrary(Module *module,
                              const std::string &libraryName) {
  KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "Linking file " << libraryName << "\n");
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
  if (!sys::fs::exists(libraryName)) {
    klee_error("Link with library %s failed. No such file.",
        libraryName.c_str());
  }

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
  ErrorOr<std::unique_ptr<MemoryBuffer> > bufferErr =
      MemoryBuffer::getFile(libraryName);
  std::error_code ec = bufferErr.getError();
#else
  OwningPtr<MemoryBuffer> Buffer;
  error_code ec = MemoryBuffer::getFile(libraryName,Buffer);
#endif
  if (ec) {
    klee_error("Link with library %s failed: %s", libraryName.c_str(),
        ec.message().c_str());
  }

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
  MemoryBufferRef Buffer = bufferErr.get()->getMemBufferRef();
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
  MemoryBuffer *Buffer = bufferErr->get();
#endif

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
  sys::fs::file_magic magic = sys::fs::identify_magic(Buffer.getBuffer());
#else
  sys::fs::file_magic magic = sys::fs::identify_magic(Buffer->getBuffer());
#endif

  LLVMContext &Context = module->getContext();
  std::string ErrorMessage;

  if (magic == sys::fs::file_magic::bitcode) {
    Module *Result = 0;
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
    ErrorOr<Module *> ResultErr = parseBitcodeFile(Buffer, Context);
    if ((ec = ResultErr.getError())) {
      ErrorMessage = ec.message();
#else
    Result = ParseBitcodeFile(Buffer.get(), Context, &ErrorMessage);
    if (!Result) {
#endif
      klee_error("Link with library %s failed: %s", libraryName.c_str(),
          ErrorMessage.c_str());
    }

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
    Result = ResultErr.get();
#endif

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
    if (Linker::LinkModules(module, Result)) {
      ErrorMessage = "linking error";
#else
    if (Linker::LinkModules(module, Result, Linker::DestroySource, &ErrorMessage)) {
#endif
      klee_error("Link with library %s failed: %s", libraryName.c_str(),
          ErrorMessage.c_str());
    }

    delete Result;

  } else if (magic == sys::fs::file_magic::archive) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
    ErrorOr<std::unique_ptr<object::Binary> > arch =
        object::createBinary(Buffer, &Context);
    ec = arch.getError();
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
    ErrorOr<object::Binary *> arch =
        object::createBinary(std::move(bufferErr.get()), &Context);
    ec = arch.getError();
#else
    OwningPtr<object::Binary> arch;
    ec = object::createBinary(Buffer.take(), arch);
#endif
    if (ec)
      klee_error("Link with library %s failed: %s", libraryName.c_str(),
          ec.message().c_str());

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
    if (object::Archive *a = dyn_cast<object::Archive>(arch->get())) {
#else
    if (object::Archive *a = dyn_cast<object::Archive>(arch.get())) {
#endif
      // Handle in helper
      if (!linkBCA(a, module, ErrorMessage))
        klee_error("Link with library %s failed: %s", libraryName.c_str(),
            ErrorMessage.c_str());
    }
    else {
    	klee_error("Link with library %s failed: Cast to archive failed", libraryName.c_str());
    }

  } else if (magic.is_object()) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
    std::unique_ptr<object::Binary> obj;
#else
    OwningPtr<object::Binary> obj;
#endif
    if (obj.get()->isObject()) {
      klee_warning("Link with library: Object file %s in archive %s found. "
          "Currently not supported.",
          obj.get()->getFileName().data(), libraryName.c_str());
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

Function *klee::getDirectCallTarget(CallSite cs, bool moduleIsFullyLinked) {
  Value *v = cs.getCalledValue();
  bool viaConstantExpr = false;
  // Walk through aliases and bitcasts to try to find
  // the function being called.
  do {
    if (Function *f = dyn_cast<Function>(v)) {
      return f;
    } else if (llvm::GlobalAlias *ga = dyn_cast<GlobalAlias>(v)) {
      if (moduleIsFullyLinked || !(ga->mayBeOverridden())) {
        v = ga->getAliasee();
      } else {
        v = NULL;
      }
    } else if (llvm::ConstantExpr *ce = dyn_cast<llvm::ConstantExpr>(v)) {
      viaConstantExpr = true;
      v = ce->getOperand(0)->stripPointerCasts();
    } else {
      v = NULL;
    }
  } while (v != NULL);

  // NOTE: This assert may fire, it isn't necessarily a problem and
  // can be disabled, I just wanted to know when and if it happened.
  assert((!viaConstantExpr) &&
         "FIXME: Unresolved direct target for a constant expression");
  return NULL;
}

static bool valueIsOnlyCalled(const Value *v) {
  for (Value::const_use_iterator it = v->use_begin(), ie = v->use_end();
       it != ie; ++it) {
    if (const Instruction *instr = dyn_cast<Instruction>(*it)) {
      if (instr->getOpcode()==0) continue; // XXX function numbering inst

      // Make sure the instruction is a call or invoke.
      CallSite cs(const_cast<Instruction *>(instr));
      if (!cs) return false;

      // Make sure that the value is only the target of this call and
      // not an argument.
      if (cs.hasArgument(v))
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

#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)

Module *klee::loadModule(LLVMContext &ctx, const std::string &path, std::string &errorMsg) {
    OwningPtr<MemoryBuffer> bufferPtr;
    error_code ec = MemoryBuffer::getFileOrSTDIN(path.c_str(), bufferPtr);
    if (ec) {
        errorMsg = ec.message();
        return 0;
    }

    Module *module = getLazyBitcodeModule(bufferPtr.get(), ctx, &errorMsg);

    if (!module) {
        return 0;
    }
    if (module->MaterializeAllPermanently(&errorMsg)) {
        delete module;
        return 0;
    }

    // In the case of success LLVM will take ownership of the module.
    // Therefore we need to take ownership away from the `bufferPtr` otherwise the
    // allocated memory will be deleted twice.
    bufferPtr.take();

    errorMsg = "";
    return module;
}

#else

Module *klee::loadModule(LLVMContext &ctx, const std::string &path, std::string &errorMsg) {
  auto buffer = MemoryBuffer::getFileOrSTDIN(path.c_str());
  if (!buffer) {
    errorMsg = buffer.getError().message().c_str();
    return nullptr;
  }

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
  auto errorOrModule = getLazyBitcodeModule(std::move(buffer.get()), ctx);
#else
  auto errorOrModule = getLazyBitcodeModule(buffer->get(), ctx);
#endif

  if (!errorOrModule) {
    errorMsg = errorOrModule.getError().message().c_str();
    return nullptr;
  }
  // The module has taken ownership of the MemoryBuffer so release it
  // from the std::unique_ptr
  buffer->release();
  auto module = *errorOrModule;

  if (auto ec = module->materializeAllPermanently()) {
    errorMsg = ec.message();
    return nullptr;
  }

  errorMsg = "";
  return module;
}
#endif
