//===-- InstructionOperandTypeCheckPass.cpp ---------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "Passes.h"
#include "klee/Config/Version.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

void printOperandWarning(const char *expected, const Instruction *i, Type *ty,
                         unsigned opNum) {
  std::string msg;
  llvm::raw_string_ostream ss(msg);
  ss << "Found unexpected type (" << *ty << ") at operand " << opNum
     << ". Expected " << expected << " in " << *i;
  i->print(ss);
  ss.flush();
  klee::klee_warning("%s", msg.c_str());
}

bool checkOperandTypeIsScalarInt(const Instruction *i, unsigned opNum) {
  assert(opNum < i->getNumOperands());
  llvm::Type *ty = i->getOperand(opNum)->getType();
  if (!(ty->isIntegerTy())) {
    printOperandWarning("scalar integer", i, ty, opNum);
    return false;
  }
  return true;
}

bool checkOperandTypeIsScalarIntOrPointer(const Instruction *i,
                                          unsigned opNum) {
  assert(opNum < i->getNumOperands());
  llvm::Type *ty = i->getOperand(opNum)->getType();
  if (!(ty->isIntegerTy() || ty->isPointerTy())) {
    printOperandWarning("scalar integer or pointer", i, ty, opNum);
    return false;
  }
  return true;
}

bool checkOperandTypeIsScalarPointer(const Instruction *i, unsigned opNum) {
  assert(opNum < i->getNumOperands());
  llvm::Type *ty = i->getOperand(opNum)->getType();
  if (!(ty->isPointerTy())) {
    printOperandWarning("scalar pointer", i, ty, opNum);
    return false;
  }
  return true;
}

bool checkOperandTypeIsScalarFloat(const Instruction *i, unsigned opNum) {
  assert(opNum < i->getNumOperands());
  llvm::Type *ty = i->getOperand(opNum)->getType();
  if (!(ty->isFloatingPointTy())) {
    printOperandWarning("scalar float", i, ty, opNum);
    return false;
  }
  return true;
}

bool checkOperandsHaveSameType(const Instruction *i, unsigned opNum0,
                               unsigned opNum1) {
  assert(opNum0 < i->getNumOperands());
  assert(opNum1 < i->getNumOperands());
  llvm::Type *ty0 = i->getOperand(opNum0)->getType();
  llvm::Type *ty1 = i->getOperand(opNum1)->getType();
  if (!(ty0 == ty1)) {
    std::string msg;
    llvm::raw_string_ostream ss(msg);
    ss << "Found mismatched type (" << *ty0 << " != " << *ty1
       << ") for operands" << opNum0 << " and " << opNum1
       << ". Expected operand types to match in " << *i;
    i->print(ss);
    ss.flush();
    klee::klee_warning("%s", msg.c_str());
    return false;
  }
  return true;
}

bool checkInstruction(const Instruction *i) {
  switch (i->getOpcode()) {
  case Instruction::Select: {
    // Note we do not enforce that operand 1 and 2 are scalar because the
    // scalarizer pass might not remove these. This could be selecting which
    // vector operand to feed to another instruction. The Executor can handle
    // this so case so this is not a problem
    return checkOperandTypeIsScalarInt(i, 0) &
           checkOperandsHaveSameType(i, 1, 2);
  }
  // Integer arithmetic, logical and shifting
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr: {
    return checkOperandTypeIsScalarInt(i, 0) &
           checkOperandTypeIsScalarInt(i, 1);
  }
  // Integer comparison
  case Instruction::ICmp: {
    return checkOperandTypeIsScalarIntOrPointer(i, 0) &
           checkOperandTypeIsScalarIntOrPointer(i, 1);
  }
  // Integer Conversion
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
  case Instruction::IntToPtr: {
    return checkOperandTypeIsScalarInt(i, 0);
  }
  case Instruction::PtrToInt: {
    return checkOperandTypeIsScalarPointer(i, 0);
  }
  // TODO: Figure out if Instruction::BitCast needs checking
  // Floating point arithmetic
  case Instruction::FAdd:
  case Instruction::FSub:
  case Instruction::FMul:
  case Instruction::FDiv:
  case Instruction::FRem: {
    return checkOperandTypeIsScalarFloat(i, 0) &
           checkOperandTypeIsScalarFloat(i, 1);
  }
  // Floating point conversion
  case Instruction::FPTrunc:
  case Instruction::FPExt:
  case Instruction::FPToUI:
  case Instruction::FPToSI: {
    return checkOperandTypeIsScalarFloat(i, 0);
  }
  case Instruction::UIToFP:
  case Instruction::SIToFP: {
    return checkOperandTypeIsScalarInt(i, 0);
  }
  // Floating point comparison
  case Instruction::FCmp: {
    return checkOperandTypeIsScalarFloat(i, 0) &
           checkOperandTypeIsScalarFloat(i, 1);
  }
  default:
    // Treat all other instructions as conforming
    return true;
  }
}
}

namespace klee {

char InstructionOperandTypeCheckPass::ID = 0;

bool InstructionOperandTypeCheckPass::runOnModule(Module &M) {
  instructionOperandsConform = true;
  for (Module::iterator fi = M.begin(), fe = M.end(); fi != fe; ++fi) {
    for (Function::iterator bi = fi->begin(), be = fi->end(); bi != be; ++bi) {
      for (BasicBlock::iterator ii = bi->begin(), ie = bi->end(); ii != ie;
           ++ii) {
        Instruction *i = &*ii;
        instructionOperandsConform &= checkInstruction(i);
      }
    }
  }

  return false;
}
}
