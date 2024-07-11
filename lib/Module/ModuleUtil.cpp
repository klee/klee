//===-- ModuleUtil.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Support/ModuleUtil.h"

#include "klee/Support/Debug.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/BinaryFormat/Magic.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"

#include <string>
#include <unordered_set>

using namespace llvm;
using namespace klee;

bool klee::linkModules(llvm::Module *composite,
                       std::vector<std::unique_ptr<llvm::Module>> &modules,
                       const unsigned flags, std::string &errorMsg) {
  //  assert(composite);

  Linker linker(*composite);
  for (auto &module : modules) {
    if (!module)
      continue;
    errorMsg = "Linking module " + module->getModuleIdentifier() + " failed";
    if (linker.linkInModule(std::move(module), flags)) {
      // Linking failed
      errorMsg = "Linking archive module with composite failed:" + errorMsg;
      return false;
    }
  }
  modules.clear();
  return true;
}

Function *klee::getDirectCallTarget(const CallBase &cb,
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
  (void)viaConstantExpr;
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

bool klee::functionEscapes(const Function *f) { return !valueIsOnlyCalled(f); }

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
    Expected<std::unique_ptr<object::Binary>> archOwner =
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
           AI != AE; ++AI) {

        StringRef memberName;
        std::error_code ec;
        ErrorOr<object::Archive::Child> childOrErr = *AI;
        ec = childOrErr.getError();
        if (ec) {
          errorMsg = ec.message();
          return false;
        }
        auto memberNameErr = childOrErr->getName();
        ec = memberNameErr ? std::error_code()
                           : errorToErrorCode(memberNameErr.takeError());
        if (!ec) {
          memberName = memberNameErr.get();
          KLEE_DEBUG_WITH_TYPE("klee_linker", dbgs()
                                                  << "Loading archive member "
                                                  << memberName << "\n");
        } else {
          errorMsg = "Archive member does not have a name!\n";
          return false;
        }

        Expected<std::unique_ptr<llvm::object::Binary>> child =
            childOrErr->getAsBinary();
        if (!child)
          ec = errorToErrorCode(child.takeError());
        if (ec) {
          // If we can't open as a binary object file its hopefully a bitcode
          // file
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

bool klee::loadFileAsOneModule(
    const std::string &libraryName, LLVMContext &context,
    std::vector<std::unique_ptr<llvm::Module>> &modules,
    std::string &errorMsg) {
  std::vector<std::unique_ptr<llvm::Module>> modules2;
  bool res = klee::loadFile(libraryName, context, modules2, errorMsg);
  if (res) {
    modules.push_back(std::move(modules2.front()));
    return linkModules(modules.back().get(), modules2, 0, errorMsg);
  }
  return res;
}

bool klee::loadArchiveAsOneModule(
    const std::string &libraryName, LLVMContext &context,
    std::vector<std::unique_ptr<llvm::Module>> &modules,
    Interpreter::FunctionsByModule &functionsByModule, std::string &errorMsg) {
  std::vector<std::unique_ptr<llvm::Module>> modules2;
  bool res = klee::loadFile(libraryName, context, modules2, errorMsg);
  if (res) {
    std::vector<std::vector<std::pair<std::string, unsigned>>> namesByModule;
    for (const auto &mod : modules2) {
      llvm::CallGraph cg(*mod);
      std::vector<std::pair<std::string, unsigned>> names;
      for (const auto &f : *mod) {
        if (!f.isDeclaration()) {
          names.push_back({f.getName().str(), cg[&f]->getNumReferences()});
        }
      }
      namesByModule.push_back(std::move(names));
    }

    modules.push_back(std::move(modules2.front()));
    res = linkModules(modules.back().get(), modules2, 0, errorMsg);
    auto mainMod = modules.front().get();
    for (const auto &mod : namesByModule) {
      std::unordered_set<llvm::Function *> setFns;
      std::vector<llvm::Function *> fns;
      for (const auto &name : mod) {
        auto fn = mainMod->getFunction(name.first);
        if (fn) {
          setFns.insert(fn);
          fns.push_back(fn);
          functionsByModule.usesInModule[fn] = name.second;
        }
      }
      functionsByModule.setModules.push_back(std::move(setFns));
      functionsByModule.modules.push_back(std::move(fns));
    }
  }
  return res;
}

void klee::replaceOrRenameFunction(llvm::Module *module, const char *old_name,
                                   const char *new_name) {
  Function *new_function, *old_function;
  new_function = module->getFunction(new_name);
  old_function = module->getFunction(old_name);
  if (old_function) {
    if (new_function) {
      old_function->replaceAllUsesWith(new_function);
      old_function->eraseFromParent();
    } else {
      old_function->setName(new_name);
      assert(old_function->getName() == new_name);
    }
  }
}
