//===-- ModuleUtil.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Support/ModuleUtil.h"

#include "klee/Config/Version.h"
#include "klee/Support/Debug.h"
#include "klee/Support/ErrorHandling.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/BinaryFormat/Magic.h"
#include "llvm/Bitcode/BitcodeReader.h"
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
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
DISABLE_WARNING_POP

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
        UndefinedSymbols.insert(Function.getName().str());
      else if (!Function.hasLocalLinkage()) {
        assert(!Function.hasDLLImportStorageClass() &&
               "Found dllimported non-external symbol!");
        DefinedSymbols.insert(Function.getName().str());
      }
    }
  }

  for (Module::const_global_iterator I = M->global_begin(), E = M->global_end();
       I != E; ++I)
    if (I->hasName()) {
      if (I->isDeclaration())
        UndefinedSymbols.insert(I->getName().str());
      else if (!I->hasLocalLinkage()) {
        assert(!I->hasDLLImportStorageClass() && "Found dllimported non-external symbol!");
        DefinedSymbols.insert(I->getName().str());
      }
    }

  for (Module::const_alias_iterator I = M->alias_begin(), E = M->alias_end();
       I != E; ++I)
    if (I->hasName())
      DefinedSymbols.insert(I->getName().str());

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

  auto containsUsedSymbols = [](const llvm::Module *module) {
    GlobalValue *GV =
        dyn_cast_or_null<GlobalValue>(module->getNamedValue("llvm.used"));
    if (!GV)
      return false;
    KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs() << "Used attribute in "
                                               << module->getModuleIdentifier()
                                               << '\n');
    return true;
  };

  for (auto &module : modules) {
    if (!module || !containsUsedSymbols(module.get()))
      continue;
    if (!linkTwoModules(composite.get(), std::move(module), errorMsg)) {
      // Linking failed
      errorMsg = "Linking module containing '__attribute__((used))'"
                 " symbols with composite failed:" +
                 errorMsg;
      return nullptr;
    }
    module = nullptr;
  }

  bool symbolsLinked = true;
  while (symbolsLinked) {
    symbolsLinked = false;
    std::set<std::string> undefinedSymbols;
    GetAllUndefinedSymbols(composite.get(), undefinedSymbols);
    auto hasRequiredDefinition = [&undefinedSymbols](
                                     const llvm::Module *module) {
      for (auto symbol : undefinedSymbols) {
        GlobalValue *GV =
            dyn_cast_or_null<GlobalValue>(module->getNamedValue(symbol));
        if (GV && !GV->isDeclaration()) {
          KLEE_DEBUG_WITH_TYPE("klee_linker",
                               dbgs() << "Found " << GV->getName() << " in "
                                      << module->getModuleIdentifier() << "\n");
          return true;
        }
      }
      return false;
    };

    // Stop in nothing is undefined
    if (undefinedSymbols.empty())
      break;

    for (auto &module : modules) {
      if (!module)
        continue;

      if (!hasRequiredDefinition(module.get()))
        continue;

      if (!linkTwoModules(composite.get(), std::move(module), errorMsg)) {
        // Linking failed
        errorMsg = "Linking archive module with composite failed:" + errorMsg;
        return nullptr;
      }
      module = nullptr;
      symbolsLinked = true;
    }
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

Function *klee::getDirectCallTarget(
    const CallBase &cb,
    bool moduleIsFullyLinked) {
  Value *v = cb.getCalledOperand();
  bool viaConstantExpr = false;
  // Walk through aliases and bitcasts to try to find
  // the function being called.
  do {
    if (isa<llvm::GlobalVariable>(v)) {
      // We don't care how we got this GlobalVariable
      viaConstantExpr = false;

      // Global variables won't be a direct call target. Instead, their
      // value need to be read and is handled as indirect call target.
      v = nullptr;
    } else if (Function *f = dyn_cast<Function>(v)) {
      return f;
    } else if (llvm::GlobalAlias *ga = dyn_cast<GlobalAlias>(v)) {
      if (moduleIsFullyLinked || !(ga->isInterposable())) {
        v = ga->getAliasee();
      } else {
        v = nullptr;
      }
    } else if (llvm::ConstantExpr *ce = dyn_cast<llvm::ConstantExpr>(v)) {
      viaConstantExpr = true;
      v = ce->getOperand(0)->stripPointerCasts();
    } else {
      v = nullptr;
    }
  } while (v != nullptr);

  // NOTE: This assert may fire, it isn't necessarily a problem and
  // can be disabled, I just wanted to know when and if it happened.
  (void) viaConstantExpr;
  assert((!viaConstantExpr) &&
         "FIXME: Unresolved direct target for a constant expression");
  return nullptr;
}

static bool valueIsOnlyCalled(const Value *v) {
  for (auto user : v->users()) {
    // Make sure the instruction is a call or invoke.
    if (const auto *cb_ptr = dyn_cast<CallBase>(user)) {
      const CallBase &cb = *cb_ptr;

      // Make sure that the value is only the target of this call and
      // not an argument.
      if (cb.hasArgument(v))
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
  file_magic magic = identify_magic(Buffer.getBuffer());

  if (magic == file_magic::bitcode) {
    SMDiagnostic Err;
    std::unique_ptr<llvm::Module> module(parseIR(Buffer, Err, context));
    if (!module) {
      klee_error("Loading file %s failed: %s", fileName.c_str(),
                 Err.getMessage().str().c_str());
    }
    modules.push_back(std::move(module));
    return true;
  }

  if (magic == file_magic::archive) {
    Expected<std::unique_ptr<object::Binary> > archOwner =
      object::createBinary(Buffer, &context);
    if (!archOwner)
      ec = errorToErrorCode(archOwner.takeError());
    llvm::object::Binary *arch = archOwner.get().get();
    if (ec)
      klee_error("Loading file %s failed: %s", fileName.c_str(),
                 ec.message().c_str());

    if (auto archive = dyn_cast<object::Archive>(arch)) {
// Load all bitcode files into memory
      auto Err = Error::success();
      for (auto AI = archive->child_begin(Err), AE = archive->child_end();
           AI != AE; ++AI)
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
        ec = memberNameErr ? std::error_code() :
                errorToErrorCode(memberNameErr.takeError());
        if (!ec) {
          memberName = memberNameErr.get();
          KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs()
                                                  << "Loading archive member "
                                                  << memberName << "\n");
        } else {
          errorMsg = "Archive member does not have a name!\n";
          return false;
        }

        Expected<std::unique_ptr<llvm::object::Binary> > child =
            childOrErr->getAsBinary();
        if (!child)
          ec = errorToErrorCode(child.takeError());
        if (ec) {
// If we can't open as a binary object file its hopefully a bitcode file
          auto buff = childOrErr->getMemoryBufferRef();
          ec = buff ? std::error_code() : errorToErrorCode(buff.takeError());
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
      if (Err) {
        errorMsg = "Cannot iterate over archive";
        return false;
      }
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
