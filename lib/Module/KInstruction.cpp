//===-- KInstruction.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/LocationInfo.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
DISABLE_WARNING_POP

#include <string>

using namespace llvm;
using namespace klee;

/***/

static int getOperandNum(
    Value *v,
    const std::unordered_map<Instruction *, unsigned> &instructionToRegisterMap,
    KModule *km, KInstruction *ki) {
  if (Instruction *inst = dyn_cast<Instruction>(v)) {
    return instructionToRegisterMap.at(inst);
  } else if (Argument *a = dyn_cast<Argument>(v)) {
    return a->getArgNo();
  } else if (isa<BasicBlock>(v) || isa<InlineAsm>(v) ||
             isa<MetadataAsValue>(v)) {
    return -1;
  } else {
    assert(isa<Constant>(v));
    Constant *c = cast<Constant>(v);
    return -(km->getConstantID(c, ki) + 2);
  }
}

KInstruction::KInstruction(
    const std::unordered_map<llvm::Instruction *, unsigned>
        &_instructionToRegisterMap,
    llvm::Instruction *_inst, KModule *_km, KBlock *_kb,
    unsigned &_globalIndexInc)
    : inst(_inst), parent(_kb), globalIndex(_globalIndexInc++) {
  if (isa<llvm::CallInst>(inst) || isa<llvm::InvokeInst>(inst)) {
    const llvm::CallBase &cs = cast<llvm::CallBase>(*inst);
    Value *val = cs.getCalledOperand();
    unsigned numArgs = cs.arg_size();
    operands = new int[numArgs + 1];
    operands[0] = getOperandNum(val, _instructionToRegisterMap, _km, this);
    for (unsigned j = 0; j < numArgs; j++) {
      Value *v = cs.getArgOperand(j);
      operands[j + 1] = getOperandNum(v, _instructionToRegisterMap, _km, this);
    }
  } else {
    unsigned numOperands = inst->getNumOperands();
    operands = new int[numOperands];
    for (unsigned j = 0; j < numOperands; j++) {
      Value *v = inst->getOperand(j);
      operands[j] = getOperandNum(v, _instructionToRegisterMap, _km, this);
    }
  }
}

KInstruction::~KInstruction() { delete[] operands; }

size_t KInstruction::getLine() const {
  auto locationInfo = getLocationInfo(inst);
  return locationInfo.line;
}

size_t KInstruction::getColumn() const {
  auto locationInfo = getLocationInfo(inst);
  return locationInfo.column;
}

std::string KInstruction::getSourceFilepath() const {
  auto locationInfo = getLocationInfo(inst);
  return locationInfo.file;
}

std::string KInstruction::getSourceLocationString() const {
  std::string filePath = getSourceFilepath();
  if (!filePath.empty()) {
    // TODO change format to file:line:column
    return filePath + ":" + std::to_string(getLine()) + " " +
           std::to_string(getColumn());
  } else {
    return "[no debug info]";
  }
}

std::string KInstruction::toString() const {
  return llvm::utostr(getIndex()) + " at " + parent->toString() + " (" +
         inst->getOpcodeName() + ")";
}

unsigned KInstruction::getGlobalIndex() const { return globalIndex; }

unsigned KInstruction::getIndex() const {
  return getGlobalIndex() - getKFunction()->getGlobalIndex() -
         getKBlock()->getId() - 1;
}

unsigned KInstruction::getDest() const {
  return parent->parent->getNumArgs() + getIndex() +
         (parent->instructions - parent->parent->instructions);
}
