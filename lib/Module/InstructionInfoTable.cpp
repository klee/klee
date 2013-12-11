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

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#else
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Module.h"
#endif
#include "llvm/Linker.h"
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 2)
#include "llvm/DebugInfo.h"
#else
#include "llvm/Analysis/DebugInfo.h"
#endif
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/Debug.h"

#include <map>
#include <string>

using namespace llvm;
using namespace klee;

class InstructionToLineAnnotator : public llvm::AssemblyAnnotationWriter {
public:
  void emitInstructionAnnot(const Instruction *i,
                            llvm::formatted_raw_ostream &os) {
    os << "%%%";
    os << (uintptr_t) i;
  }
};
        
static void buildInstructionToLineMap(Module *m,
                                      std::map<const Instruction*, unsigned> &out) {  
  InstructionToLineAnnotator a;
  std::string str;
  llvm::raw_string_ostream os(str);
  m->print(os, &a);
  os.flush();
  const char *s;

  unsigned line = 1;
  for (s=str.c_str(); *s; s++) {
    if (*s=='\n') {
      line++;
      if (s[1]=='%' && s[2]=='%' && s[3]=='%') {
        s += 4;
        char *end;
        unsigned long long value = strtoull(s, &end, 10);
        if (end!=s) {
          out.insert(std::make_pair((const Instruction*) value, line));
        }
        s = end;
      }
    }
  }
}

static std::string getDSPIPath(DILocation Loc) {
  std::string dir = Loc.getDirectory();
  std::string file = Loc.getFilename();
  if (dir.empty() || file[0] == '/') {
    return file;
  } else if (*dir.rbegin() == '/') {
    return dir + file;
  } else {
    return dir + "/" + file;
  }
}

bool InstructionInfoTable::getInstructionDebugInfo(const llvm::Instruction *I, 
                                                   const std::string *&File,
                                                   unsigned &Line) {
  if (MDNode *N = I->getMetadata("dbg")) {
    DILocation Loc(N);
    File = internString(getDSPIPath(Loc));
    Line = Loc.getLineNumber();
    return true;
  }

  return false;
}

InstructionInfoTable::InstructionInfoTable(Module *m) 
  : dummyString(""), dummyInfo(0, dummyString, 0, 0) {
  unsigned id = 0;
  std::map<const Instruction*, unsigned> lineTable;
  buildInstructionToLineMap(m, lineTable);

  for (Module::iterator fnIt = m->begin(), fn_ie = m->end(); 
       fnIt != fn_ie; ++fnIt) {

    for (inst_iterator it = inst_begin(fnIt), ie = inst_end(fnIt); it != ie;
        ++it) {
      const std::string *initialFile = &dummyString;
      unsigned initialLine = 0;
      Instruction *instr = &*it;
      unsigned assemblyLine = 0;

      std::map<const Instruction*, unsigned>::const_iterator ltit =
        lineTable.find(instr);
      if (ltit!=lineTable.end())
        assemblyLine = ltit->second;
      if (getInstructionDebugInfo(instr, initialFile, initialLine))
      {
        infos.insert(std::make_pair(instr,
                                    InstructionInfo(id++,
                                                    *initialFile,
                                                    initialLine,
                                                    assemblyLine)));
        DEBUG_WITH_TYPE("klee_obtained_debug", dbgs() <<
          "Instruction: \"" << *instr << "\" (assembly line " << assemblyLine <<
          ") has debug location " << *initialFile << ":" << initialLine << "\n");
      }
      else
      {
        DEBUG_WITH_TYPE("klee_missing_debug", dbgs() <<
          "Instruction: \"" << *instr << "\" (assembly line " << assemblyLine <<
          ") is missing debug info.\n");
      }
    }
  }
}

InstructionInfoTable::~InstructionInfoTable() {
  for (std::set<const std::string *, ltstr>::iterator
         it = internedStrings.begin(), ie = internedStrings.end();
       it != ie; ++it)
    delete *it;
}

const std::string *InstructionInfoTable::internString(std::string s) {
  std::set<const std::string *, ltstr>::iterator it = internedStrings.find(&s);
  if (it==internedStrings.end()) {
    std::string *interned = new std::string(s);
    internedStrings.insert(interned);
    return interned;
  } else {
    return *it;
  }
}

unsigned InstructionInfoTable::getMaxID() const {
  return infos.size();
}

const InstructionInfo &
InstructionInfoTable::getInfo(const Instruction *inst) const {
  std::map<const llvm::Instruction*, InstructionInfo>::const_iterator it = 
    infos.find(inst);
  if (it==infos.end()) {
    return dummyInfo;
  } else {
    return it->second;
  }
}

const InstructionInfo &
InstructionInfoTable::getFunctionInfo(const Function *f) const {
  if (f->isDeclaration()) {
    return dummyInfo;
  } else {
    return getInfo(f->begin()->begin());
  }
}
