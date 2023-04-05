//===-- InstructionInfoTable.cpp ------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/InstructionInfoTable.h"
#include "klee/Config/Version.h"

#include "StreamWithLine.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/ADT/Optional.h>

#include <cstdint>
#include <map>
#include <string>

using namespace klee;

class InstructionToLineAnnotator : public llvm::AssemblyAnnotationWriter {
public:
  void emitInstructionAnnot(const llvm::Instruction *i,
                            llvm::formatted_raw_ostream &os) override {
    os << "%%%" + std::to_string(reinterpret_cast<std::uintptr_t>(i));
  }

  void emitFunctionAnnot(const llvm::Function *f,
                         llvm::formatted_raw_ostream &os) override {
    os << "%%%" + std::to_string(reinterpret_cast<std::uintptr_t>(f));
  }
};

static std::unordered_map<uintptr_t, uint64_t>
buildInstructionToLineMap(const llvm::Module &m,
                          std::unique_ptr<llvm::raw_fd_ostream> assemblyFS) {

  std::unordered_map<uintptr_t, uint64_t> mapping;
  InstructionToLineAnnotator a;

  StreamWithLine os(std::move(assemblyFS));
  m.print(os, &a);
  os.flush();

  return os.getMapping();
}

class DebugInfoExtractor {
  std::vector<std::unique_ptr<std::string>> &internedStrings;
  std::unordered_map<uintptr_t, uint64_t> lineTable;

  const llvm::Module &module;
  bool withAsm = false;

public:
  DebugInfoExtractor(
      std::vector<std::unique_ptr<std::string>> &_internedStrings,
      const llvm::Module &_module,
      std::unique_ptr<llvm::raw_fd_ostream> assemblyFS)
      : internedStrings(_internedStrings), module(_module) {
    if (assemblyFS) {
      withAsm = true;
      lineTable = buildInstructionToLineMap(module, std::move(assemblyFS));
    }
  }

  std::string &getInternedString(const std::string &s) {
    auto found = std::find_if(internedStrings.begin(), internedStrings.end(),
                              [&s](const std::unique_ptr<std::string> &item) {
                                return *item.get() == s;
                              });
    if (found != internedStrings.end())
      return *found->get();

    auto newItem = std::unique_ptr<std::string>(new std::string(s));
    auto result = newItem.get();

    internedStrings.emplace_back(std::move(newItem));
    return *result;
  }

  std::unique_ptr<FunctionInfo> getFunctionInfo(const llvm::Function &Func) {
    llvm::Optional<uint64_t> asmLine;
    if (withAsm) {
      asmLine = lineTable.at(reinterpret_cast<std::uintptr_t>(&Func));
    }
    auto dsub = Func.getSubprogram();

    if (dsub != nullptr) {
      auto path = dsub->getFilename();
      return std::make_unique<FunctionInfo>(FunctionInfo(
          0, getInternedString(path.str()), dsub->getLine(), asmLine));
    }

    // Fallback: Mark as unknown
    return std::make_unique<FunctionInfo>(
        FunctionInfo(0, getInternedString(""), 0, asmLine));
  }

  std::unique_ptr<InstructionInfo>
  getInstructionInfo(const llvm::Instruction &Inst, const FunctionInfo *f) {
    llvm::Optional<uint64_t> asmLine;
    if (withAsm) {
      asmLine = lineTable.at(reinterpret_cast<std::uintptr_t>(&Inst));
    }

    // Retrieve debug information associated with instruction
    auto dl = Inst.getDebugLoc();

    // Check if a valid debug location is assigned to the instruction.
    if (dl.get() != nullptr) {
      auto full_path = dl.get()->getFilename();
      auto line = dl.getLine();
      auto column = dl.getCol();

      // Still, if the line is unknown, take the context of the instruction to
      // narrow it down
      if (line == 0) {
        if (auto LexicalBlock =
                llvm::dyn_cast<llvm::DILexicalBlock>(dl.getScope())) {
          line = LexicalBlock->getLine();
          column = LexicalBlock->getColumn();
        }
      }
      return std::make_unique<InstructionInfo>(InstructionInfo(
          0, getInternedString(full_path.str()), line, column, asmLine));
    }

    if (f != nullptr)
      // If nothing found, use the surrounding function
      return std::make_unique<InstructionInfo>(
          InstructionInfo(0, f->file, f->line, 0, asmLine));
    // If nothing found, use the surrounding function
    return std::make_unique<InstructionInfo>(
        InstructionInfo(0, getInternedString(""), 0, 0, asmLine));
  }
};

InstructionInfoTable::InstructionInfoTable(
    const llvm::Module &m, std::unique_ptr<llvm::raw_fd_ostream> assemblyFS) {
  // Generate all debug instruction information
  DebugInfoExtractor DI(internedStrings, m, std::move(assemblyFS));

  for (const auto &Func : m) {
    auto F = DI.getFunctionInfo(Func);
    auto FR = F.get();
    functionInfos.emplace(&Func, std::move(F));

    for (auto it = llvm::inst_begin(Func), ie = llvm::inst_end(Func); it != ie;
         ++it) {
      auto instr = &*it;
      infos.emplace(instr, DI.getInstructionInfo(*instr, FR));
    }
  }

  // Make sure that every item has a unique ID
  size_t idCounter = 0;
  for (auto &item : infos)
    item.second->id = idCounter++;
  for (auto &item : functionInfos)
    item.second->id = idCounter++;
}

unsigned InstructionInfoTable::getMaxID() const {
  return infos.size() + functionInfos.size();
}

const InstructionInfo &
InstructionInfoTable::getInfo(const llvm::Instruction &inst) const {
  auto it = infos.find(&inst);
  if (it == infos.end())
    llvm::report_fatal_error("invalid instruction, not present in "
                             "initial module!");
  return *it->second.get();
}

const FunctionInfo &
InstructionInfoTable::getFunctionInfo(const llvm::Function &f) const {
  auto found = functionInfos.find(&f);
  if (found == functionInfos.end())
    llvm::report_fatal_error("invalid instruction, not present in "
                             "initial module!");

  return *found->second.get();
}
