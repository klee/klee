//===-- ExecutorUtil.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Executor.h"

#include "klee/Expr.h"
#include "klee/Interpreter.h"
#include "klee/Machine.h"
#include "klee/Solver.h"

#include "klee/Internal/Module/KModule.h"

#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ModuleProvider.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/Streams.h"
#include "llvm/Target/TargetData.h"
#include <iostream>
#include <cassert>

using namespace klee;
using namespace llvm;

namespace klee {

ref<Expr> 
Executor::evalConstantExpr(llvm::ConstantExpr *ce) {
  const llvm::Type *type = ce->getType();

  ref<Expr> op1(0), op2(0), op3(0);
  int numOperands = ce->getNumOperands();

  if (numOperands > 0) op1 = evalConstant(ce->getOperand(0));
  if (numOperands > 1) op2 = evalConstant(ce->getOperand(1));
  if (numOperands > 2) op3 = evalConstant(ce->getOperand(2));
  
  switch (ce->getOpcode()) {
    default :
      assert(0 && "unknown ConstantExpr type");

    case Instruction::Trunc: return ExtractExpr::createByteOff(op1, 
							       0,
							       Expr::getWidthForLLVMType(type));
    case Instruction::ZExt:  return  ZExtExpr::create(op1, 
                                                      Expr::getWidthForLLVMType(type));
    case Instruction::SExt:  return  SExtExpr::create(op1, 
                                                      Expr::getWidthForLLVMType(type));
    case Instruction::Add:   return   AddExpr::create(op1, op2);
    case Instruction::Sub:   return   SubExpr::create(op1, op2);
    case Instruction::Mul:   return   MulExpr::create(op1, op2);
    case Instruction::SDiv:  return  SDivExpr::create(op1, op2);
    case Instruction::UDiv:  return  UDivExpr::create(op1, op2);
    case Instruction::SRem:  return  SRemExpr::create(op1, op2);
    case Instruction::URem:  return  URemExpr::create(op1, op2);
    case Instruction::And:   return   AndExpr::create(op1, op2);
    case Instruction::Or:    return    OrExpr::create(op1, op2);
    case Instruction::Xor:   return   XorExpr::create(op1, op2);
    case Instruction::Shl:   return   ShlExpr::create(op1, op2);
    case Instruction::LShr:  return  LShrExpr::create(op1, op2);
    case Instruction::AShr:  return  AShrExpr::create(op1, op2);
    case Instruction::BitCast:  return op1;

    case Instruction::IntToPtr: {
      return ZExtExpr::create(op1, Expr::getWidthForLLVMType(type));
    } 

    case Instruction::PtrToInt: {
      return ZExtExpr::create(op1, Expr::getWidthForLLVMType(type));
    }
      
    case Instruction::GetElementPtr: {
      ref<Expr> base = op1;

      for (gep_type_iterator ii = gep_type_begin(ce), ie = gep_type_end(ce);
           ii != ie; ++ii) {
        ref<Expr> addend = ConstantExpr::alloc(0, kMachinePointerType);
        
        if (const StructType *st = dyn_cast<StructType>(*ii)) {
          const StructLayout *sl = kmodule->targetData->getStructLayout(st);
          const ConstantInt *ci = cast<ConstantInt>(ii.getOperand());
          
          addend = Expr::createPointer(sl->getElementOffset((unsigned)
                                                            ci->getZExtValue()));
        } else {
          const SequentialType *st = cast<SequentialType>(*ii);
          ref<Expr> index = evalConstant(cast<Constant>(ii.getOperand()));
          unsigned elementSize = kmodule->targetData->getTypeStoreSize(st->getElementType());
          
          index = Expr::createCoerceToPointerType(index);
          addend = MulExpr::create(index,
                                   Expr::createPointer(elementSize));
        }
        
        base = AddExpr::create(base, addend);
      }
      
      return base;
    }
      
    case Instruction::ICmp: {
      switch(ce->getPredicate()) {
        case ICmpInst::ICMP_EQ:  return  EqExpr::create(op1, op2);
        case ICmpInst::ICMP_NE:  return  NeExpr::create(op1, op2);
        case ICmpInst::ICMP_UGT: return UgtExpr::create(op1, op2);
        case ICmpInst::ICMP_UGE: return UgeExpr::create(op1, op2);
        case ICmpInst::ICMP_ULT: return UltExpr::create(op1, op2);
        case ICmpInst::ICMP_ULE: return UleExpr::create(op1, op2);
        case ICmpInst::ICMP_SGT: return SgtExpr::create(op1, op2);
        case ICmpInst::ICMP_SGE: return SgeExpr::create(op1, op2);
        case ICmpInst::ICMP_SLT: return SltExpr::create(op1, op2);
        case ICmpInst::ICMP_SLE: return SleExpr::create(op1, op2);
        default:
          assert(0 && "unhandled ICmp predicate");
      }
    }
      
    case Instruction::Select: {
      return SelectExpr::create(op1, op2, op3);
    }

    case Instruction::FDiv:
    case Instruction::FRem:
    case Instruction::FPTrunc:
    case Instruction::FPExt:
    case Instruction::UIToFP:
    case Instruction::SIToFP:
    case Instruction::FPToUI:
    case Instruction::FPToSI:
    case Instruction::FCmp:
      assert(0 && "floating point ConstantExprs unsupported");
  }
}

}
