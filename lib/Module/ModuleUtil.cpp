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

#include "llvm/Analysis/ValueTracking.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
#include "llvm/BinaryFormat/Magic.h"
#endif
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/Error.h"
#include "llvm/Object/ObjectFile.h"
#if LLVM_VERSION_CODE < LLVM_VERSION(4, 0)
#include "llvm/Support/DataStream.h"
#endif
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
#include <llvm/Bitcode/BitcodeReader.h>
#else
#include <llvm/Bitcode/ReaderWriter.h>
#endif

#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>

using namespace llvm;
using namespace klee;

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
                       dbgs() << "*** Computing undefined symbols for "
                              << M->getModuleIdentifier() << " ***\n");

  for (auto const &Function : *M) {
    if (Function.hasName()) {
      if (Function.isDeclaration())
        UndefinedSymbols.insert(Function.getName());
      else if (!Function.hasLocalLinkage()) {
        assert(!Function.hasDLLImportStorageClass() &&
               "Found dllimported non-external symbol!");
        DefinedSymbols.insert(Function.getName());
      }
    }
  }

  for (Module::const_global_iterator I = M->global_begin(), E = M->global_end();
       I != E; ++I)
    if (I->hasName()) {
      if (I->isDeclaration())
        UndefinedSymbols.insert(I->getName());
      else if (!I->hasLocalLinkage()) {
        assert(!I->hasDLLImportStorageClass() && "Found dllimported non-external symbol!");
        DefinedSymbols.insert(I->getName());
      }
    }

  for (Module::const_alias_iterator I = M->alias_begin(), E = M->alias_end();
       I != E; ++I)
    if (I->hasName())
      DefinedSymbols.insert(I->getName());


  // Prune out any defined symbols from the undefined symbols set
  // and other symbols we don't want to treat as an undefined symbol
  std::vector<std::string> SymbolsToRemove;
  for (std::set<std::string>::iterator I = UndefinedSymbols.begin();
       I != UndefinedSymbols.end(); ++I )
  {
    if (DefinedSymbols.find(*I) != DefinedSymbols.end()) {
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

  // Now remove the symbols from undefined set.
  for (auto const &symbol : SymbolsToRemove)
    UndefinedSymbols.erase(symbol);

  KLEE_DEBUG_WITH_TYPE("klee_linker",
                       dbgs() << "*** Finished computing undefined symbols ***\n");
}

static bool linkTwoModules(llvm::Module *Dest,
                           std::unique_ptr<llvm::Module> Src,
                           std::string &errorMsg) {
  // Get the potential error message (Src is moved and won't be available later)
  errorMsg = "Linking module " + Src->getModuleIdentifier() + " failed";
  auto linkResult = Linker::linkModules(*Dest, std::move(Src));

  return !linkResult;
}

std::unique_ptr<llvm::Module>
klee::linkModules(std::vector<std::unique_ptr<llvm::Module>> &modules,
                  llvm::StringRef entryFunction, std::string &errorMsg) {
  assert(!modules.empty() && "modules list should not be empty");

  if (entryFunction.empty()) {
    // If no entry function is provided, link all modules together into one
    std::unique_ptr<llvm::Module> composite = std::move(modules.back());
    modules.pop_back();

    // Just link all modules together
    for (auto &module : modules) {
      if (linkTwoModules(composite.get(), std::move(module), errorMsg))
        continue;

      // Linking failed
      errorMsg = "Linking archive module with composite failed:" + errorMsg;
      return nullptr;
    }

    // clean up every module as we already linked in every module
    modules.clear();
    return composite;
  }

  // Starting from the module containing the entry function, resolve unresolved
  // dependencies recursively


  // search for the module containing the entry function
  std::unique_ptr<llvm::Module> composite;
  for (auto &module : modules) {
    if (!module || !module->getNamedValue(entryFunction))
      continue;
    if (composite) {
      errorMsg =
          "Function " + entryFunction.str() +
          " defined in different modules (" + module->getModuleIdentifier() +
          " already defined in: " + composite->getModuleIdentifier() + ")";
      return nullptr;
    }
    composite = std::move(module);
  }

  // fail if not found
  if (!composite) {
    errorMsg =
        "Entry function '" + entryFunction.str() + "' not found in module.";
    return nullptr;
  }

  while (true) {
    std::set<std::string> undefinedSymbols;
    GetAllUndefinedSymbols(composite.get(), undefinedSymbols);

    // Stop in nothing is undefined
    if (undefinedSymbols.empty())
      break;

    bool merged = false;
    for (auto &module : modules) {
      if (!module)
        continue;

      for (auto symbol : undefinedSymbols) {
        GlobalValue *GV =
            dyn_cast_or_null<GlobalValue>(module->getNamedValue(symbol));
        if (!GV || GV->isDeclaration())
          continue;

        // Found symbol, therefore merge in module
        KLEE_DEBUG_WITH_TYPE("klee_linker",
                             dbgs() << "Found " << GV->getName() << " in "
                                    << module->getModuleIdentifier() << "\n");
        if (linkTwoModules(composite.get(), std::move(module), errorMsg)) {
          module = nullptr;
          merged = true;
          break;
        }
        // Linking failed
        errorMsg = "Linking archive module with composite failed:" + errorMsg;
        return nullptr;
      }
    }
    if (!merged)
      break;
  }

  // Condense the module array
  std::vector<std::unique_ptr<llvm::Module>> LeftoverModules;
  for (auto &module : modules) {
    if (module)
      LeftoverModules.emplace_back(std::move(module));
  }

  modules.swap(LeftoverModules);
  return composite;
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
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
      if (moduleIsFullyLinked || !(ga->isInterposable())) {
#else
      if (moduleIsFullyLinked || !(ga->mayBeOverridden())) {
#endif
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
  (void) viaConstantExpr;
  assert((!viaConstantExpr) &&
         "FIXME: Unresolved direct target for a constant expression");
  return NULL;
}

static bool valueIsOnlyCalled(const Value *v) {
  for (auto user : v->users()) {
    if (const auto *instr = dyn_cast<Instruction>(user)) {
      // Make sure the instruction is a call or invoke.
      CallSite cs(const_cast<Instruction *>(instr));
      if (!cs) return false;

      // Make sure that the value is only the target of this call and
      // not an argument.
      if (cs.hasArgument(v))
        return false;
    } else if (const auto *ce = dyn_cast<ConstantExpr>(user)) {
      if (ce->getOpcode() == Instruction::BitCast)
        if (valueIsOnlyCalled(ce))
          continue;
      return false;
    } else if (const auto *ga = dyn_cast<GlobalAlias>(user)) {
      if (v == ga->getAliasee() && !valueIsOnlyCalled(ga))
        return false;
    } else if (isa<BlockAddress>(user)) {
      // only valid as operand to indirectbr or comparison against null
      continue;
    } else {
      return false;
    }
  }

  return true;
}

bool klee::functionEscapes(const Function *f) {
  return !valueIsOnlyCalled(f);
}

bool klee::loadFile(const std::string &fileName, LLVMContext &context,
                    std::vector<std::unique_ptr<llvm::Module>> &modules,
                    std::string &errorMsg) {
  KLEE_DEBUG_WITH_TYPE("klee_loader", dbgs()
                                          << "Load file " << fileName << "\n");

  ErrorOr<std::unique_ptr<MemoryBuffer>> bufferErr =
      MemoryBuffer::getFileOrSTDIN(fileName);
  std::error_code ec = bufferErr.getError();
  if (ec) {
    klee_error("Loading file %s failed: %s", fileName.c_str(),
               ec.message().c_str());
  }

  MemoryBufferRef Buffer = bufferErr.get()->getMemBufferRef();

#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
  file_magic magic = identify_magic(Buffer.getBuffer());
#else
  sys::fs::file_magic magic = sys::fs::identify_magic(Buffer.getBuffer());
#endif

#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
  if (magic == file_magic::bitcode) {
#else
  if (magic == sys::fs::file_magic::bitcode) {
#endif
    SMDiagnostic Err;
    std::unique_ptr<llvm::Module> module(parseIR(Buffer, Err, context));
    if (!module) {
      klee_error("Loading file %s failed: %s", fileName.c_str(),
                 Err.getMessage().str().c_str());
    }
    modules.push_back(std::move(module));
    return true;
  }

#if LLVM_VERSION_CODE >= LLVM_VERSION(5, 0)
  if (magic == file_magic::archive) {
#else
  if (magic == sys::fs::file_magic::archive) {
#endif
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
    Expected<std::unique_ptr<object::Binary> > archOwner =
      object::createBinary(Buffer, &context);
    if (!archOwner)
      ec = errorToErrorCode(archOwner.takeError());
    llvm::object::Binary *arch = archOwner.get().get();
#else
    ErrorOr<std::unique_ptr<object::Binary>> archOwner =
        object::createBinary(Buffer, &context);
    ec = archOwner.getError();
    llvm::object::Binary *arch = archOwner.get().get();
#endif
    if (ec)
      klee_error("Loading file %s failed: %s", fileName.c_str(),
                 ec.message().c_str());

    if (auto archive = dyn_cast<object::Archive>(arch)) {
// Load all bitcode files into memory
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
      auto Err = Error::success();
      for (auto AI = archive->child_begin(Err), AE = archive->child_end();
           AI != AE; ++AI)
#else
      for (auto AI = archive->child_begin(), AE = archive->child_end();
           AI != AE; ++AI)
#endif
      {

        StringRef memberName;
        std::error_code ec;
        ErrorOr<object::Archive::Child> childOrErr = *AI;
        ec = childOrErr.getError();
        if (ec) {
                errorMsg = ec.message();
                return false;
        }
        auto memberNameErr = childOrErr->getName();
#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
        ec = memberNameErr ? std::error_code() :
                errorToErrorCode(memberNameErr.takeError());
#else
        ec = memberNameErr.getError();
#endif
        if (!ec) {
          memberName = memberNameErr.get();
          KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs()
                                                  << "Loading archive member "
                                                  << memberName << "\n");
        } else {
          errorMsg = "Archive member does not have a name!\n";
          return false;
        }

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
        Expected<std::unique_ptr<llvm::object::Binary> > child =
            childOrErr->getAsBinary();
        if (!child)
          ec = errorToErrorCode(child.takeError());
#else
        ErrorOr<std::unique_ptr<llvm::object::Binary>> child =
            childOrErr->getAsBinary();
        ec = child.getError();
#endif
        if (ec) {
// If we can't open as a binary object file its hopefully a bitcode file
          auto buff = childOrErr->getMemoryBufferRef();
#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
          ec = buff ? std::error_code() : errorToErrorCode(buff.takeError());
#else
          ec = buff.getError();
#endif
          if (ec) {
            errorMsg = "Failed to get MemoryBuffer: " + ec.message();
            return false;
          }

          if (buff) {
            // FIXME: Maybe load bitcode file lazily? Then if we need to link,
            // materialise
            // the module
            SMDiagnostic Err;
            std::unique_ptr<llvm::Module> module =
                parseIR(buff.get(), Err, context);
            if (!module) {
              klee_error("Loading file %s failed: %s", fileName.c_str(),
                         Err.getMessage().str().c_str());
            }

            modules.push_back(std::move(module));
          } else {
            errorMsg = "Buffer was NULL!";
            return false;
          }

        } else if (child.get()->isObject()) {
          errorMsg = "Object file " + child.get()->getFileName().str() +
                     " in archive is not supported";
          return false;
        } else {
          errorMsg = "Loading archive child with error " + ec.message();
          return false;
        }
      }
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
      if (Err) {
        errorMsg = "Cannot iterate over archive";
        return false;
      }
#endif
    }

    return true;
  }
  if (magic.is_object()) {
    errorMsg = "Loading file " + fileName +
               " Object file as input is currently not supported";
    return false;
  }
  // This might still be an assembly file. Let's try to parse it.
  SMDiagnostic Err;
  std::unique_ptr<llvm::Module> module(parseIR(Buffer, Err, context));
  if (!module) {
    klee_error("Loading file %s failed: Unrecognized file type.",
               fileName.c_str());
  }
  modules.push_back(std::move(module));
  return true;
}
