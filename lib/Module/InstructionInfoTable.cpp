//===-- InstructionInfoTable.cpp ------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/Module/InstructionInfoTable.h"
#include "klee/Config/Version.h"

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

#include <cstdint>
#include <map>
#include <string>

using namespace klee;

class InstructionToLineAnnotator : public llvm::AssemblyAnnotationWriter {
public:
  void emitInstructionAnnot(const llvm::Instruction *i,
                            llvm::formatted_raw_ostream &os) {
    os << "%%%";
    os << reinterpret_cast<std::uintptr_t>(i);
  }

  void emitFunctionAnnot(const llvm::Function *f,
                         llvm::formatted_raw_ostream &os) {
    os << "%%%";
    os << reinterpret_cast<std::uintptr_t>(f);
  }
};

static std::map<uintptr_t, uint64_t>
buildInstructionToLineMap(const llvm::Module &m) {

  std::map<uintptr_t, uint64_t> mapping;
  InstructionToLineAnnotator a;
  std::string str;

  llvm::raw_string_ostream os(str);
  m.print(os, &a);
  os.flush();

  const char *s;

  unsigned line = 1;
  for (s=str.c_str(); *s; s++) {
    if (*s != '\n')
      continue;

    line++;
    if (s[1] != '%' || s[2] != '%' || s[3] != '%')
      continue;

    s += 4;
    char *end;
    uint64_t value = strtoull(s, &end, 10);
    if (end != s) {
      mapping.insert(std::make_pair(value, line));
    }
    s = end;
  }

  return mapping;
}

static std::string getFullPath(llvm::StringRef Directory,
                               llvm::StringRef FileName) {
  llvm::SmallString<128> file_pathname(Directory);
  llvm::sys::path::append(file_pathname, FileName);

  return file_pathname.str();
}

class DebugInfoExtractor {
  std::vector<std::unique_ptr<std::string>> &internedStrings;
  std::map<uintptr_t, uint64_t> lineTable;

  const llvm::Module &module;

public:
  DebugInfoExtractor(
      std::vector<std::unique_ptr<std::string>> &_internedStrings,
      const llvm::Module &_module)
      : internedStrings(_internedStrings), module(_module) {
    lineTable = buildInstructionToLineMap(module);
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
    auto asmLine = lineTable.at(reinterpret_cast<std::uintptr_t>(&Func));
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
    auto dsub = Func.getSubprogram();
#else
    auto dsub = llvm::getDISubprogram(&Func);
#endif
    if (dsub != nullptr) {
      auto path = getFullPath(dsub->getDirectory(), dsub->getFilename());
      return std::unique_ptr<FunctionInfo>(new FunctionInfo(
          0, getInternedString(path), dsub->getLine(), asmLine));
    }

    // Fallback: Mark as unknown
    return std::unique_ptr<FunctionInfo>(
        new FunctionInfo(0, getInternedString(""), 0, asmLine));
  }

  std::unique_ptr<InstructionInfo>
  getInstructionInfo(const llvm::Instruction &Inst, const FunctionInfo *f) {
    auto asmLine = lineTable.at(reinterpret_cast<std::uintptr_t>(&Inst));

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
      return std::unique_ptr<InstructionInfo>(new InstructionInfo(
          0, getInternedString(full_path), line, column, asmLine));
    }

    if (f != nullptr)
      // If nothing found, use the surrounding function
      return std::unique_ptr<InstructionInfo>(
          new InstructionInfo(0, f->file, f->line, 0, asmLine));
    // If nothing found, use the surrounding function
    return std::unique_ptr<InstructionInfo>(
        new InstructionInfo(0, getInternedString(""), 0, 0, asmLine));
  }
};

InstructionInfoTable::InstructionInfoTable(const llvm::Module &m) {
  // Generate all debug instruction information
  DebugInfoExtractor DI(internedStrings, m);
  for (const auto &Func : m) {
    auto F = DI.getFunctionInfo(Func);
    auto FR = F.get();
    functionInfos.insert(std::make_pair(&Func, std::move(F)));

    for (auto it = llvm::inst_begin(Func), ie = llvm::inst_end(Func); it != ie;
         ++it) {
      auto instr = &*it;
      infos.insert(std::make_pair(instr, DI.getInstructionInfo(*instr, FR)));
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
