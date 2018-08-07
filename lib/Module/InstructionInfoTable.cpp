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

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

# if LLVM_VERSION_CODE < LLVM_VERSION(3,5)
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Linker.h"
#else
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Linker/Linker.h"
#endif

#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_ostream.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3,5)
#include "llvm/IR/DebugInfo.h"
#else
#include "llvm/DebugInfo.h"
#endif

#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Path.h"

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
  llvm::DebugInfoFinder DIF;

  uint64_t counter;

  std::map<uintptr_t, uint64_t> lineTable;

  const llvm::Module &module;

public:
  DebugInfoExtractor(
      std::vector<std::unique_ptr<std::string>> &_internedStrings,
      const llvm::Module &_module)
      : internedStrings(_internedStrings), counter(0), module(_module) {
    DIF.processModule(module);
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

    // Acquire function debug information
    for (auto subIt = DIF.subprogram_begin(), subItE = DIF.subprogram_end();
         subIt != subItE; ++subIt) {
      llvm::DISubprogram SubProgram(*subIt);
      if (SubProgram.getFunction() != &Func)
        continue;

      auto path =
          getFullPath(SubProgram.getDirectory(), SubProgram.getFilename());

      return std::unique_ptr<FunctionInfo>(
          new FunctionInfo(counter++, getInternedString(path),
                           SubProgram.getLineNumber(), asmLine));
    }

    return std::unique_ptr<FunctionInfo>(
        new FunctionInfo(counter++, getInternedString(""), 0, asmLine));
  }

  std::unique_ptr<InstructionInfo>
  getInstructionInfo(const llvm::Instruction &Inst, const FunctionInfo &f) {
    auto asmLine = lineTable.at(reinterpret_cast<std::uintptr_t>(&Inst));

    llvm::DebugLoc Loc(Inst.getDebugLoc());
    if (!Loc.isUnknown()) {
      llvm::DIScope Scope(Loc.getScope(module.getContext()));
      auto full_path = getFullPath(Scope.getDirectory(), Scope.getFilename());
      return std::unique_ptr<InstructionInfo>(
          new InstructionInfo(counter++, getInternedString(full_path),
                              Loc.getLine(), Loc.getCol(), asmLine));
    }

    // If nothing found, use the surrounding function
    return std::unique_ptr<InstructionInfo>(
        new InstructionInfo(counter++, f.file, f.line, 0, asmLine));
  }
};

InstructionInfoTable::InstructionInfoTable(const llvm::Module &m) {
  DebugInfoExtractor DI(internedStrings, m);
  for (const auto &Func : m) {
    auto F = DI.getFunctionInfo(Func);
    auto FD = F.get();
    functionInfos.insert(std::make_pair(&Func, std::move(F)));

    for (auto it = llvm::inst_begin(Func), ie = llvm::inst_end(Func); it != ie;
         ++it) {
      auto instr = &*it;
      infos.insert(std::make_pair(instr, DI.getInstructionInfo(*instr, *FD)));
    }
  }
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
