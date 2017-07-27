//===-- ExecutorUtil.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Executor.h"

#include "Context.h"

#include "klee/Expr.h"
#include "klee/Interpreter.h"
#include "klee/Solver.h"

#include "klee/Config/Version.h"
#include "klee/Internal/Module/KModule.h"

#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/util/GetElementPtrTypeIterator.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>

using namespace klee;
using namespace llvm;

namespace klee {

  ref<klee::ConstantExpr> Executor::evalConstant(const Constant *c,
						 const KInstruction *ki) {
    if (!ki) {
      KConstant* kc = kmodule->getKConstant(c);
      if (kc)
	ki = kc->ki;
    }

    if (const llvm::ConstantExpr *ce = dyn_cast<llvm::ConstantExpr>(c)) {
      return evalConstantExpr(ce, ki);
    } else {
      if (const ConstantInt *ci = dyn_cast<ConstantInt>(c)) {
	return ConstantExpr::alloc(ci->getValue());
      } else if (const ConstantFP *cf = dyn_cast<ConstantFP>(c)) {
	return ConstantExpr::alloc(cf->getValueAPF().bitcastToAPInt());
      } else if (const GlobalValue *gv = dyn_cast<GlobalValue>(c)) {
	return globalAddresses.find(gv)->second;
      } else if (isa<ConstantPointerNull>(c)) {
	return Expr::createPointer(0);
      } else if (isa<UndefValue>(c) || isa<ConstantAggregateZero>(c)) {
	return ConstantExpr::create(0, getWidthForLLVMType(c->getType()));
      } else if (const ConstantDataSequential *cds =
                 dyn_cast<ConstantDataSequential>(c)) {
	std::vector<ref<Expr> > kids;
	for (unsigned i = 0, e = cds->getNumElements(); i != e; ++i) {
	  ref<Expr> kid = evalConstant(cds->getElementAsConstant(i), ki);
	  kids.push_back(kid);
	}
	ref<Expr> res = ConcatExpr::createN(kids.size(), kids.data());
	return cast<ConstantExpr>(res);
      } else if (const ConstantStruct *cs = dyn_cast<ConstantStruct>(c)) {
	const StructLayout *sl = kmodule->targetData->getStructLayout(cs->getType());
	llvm::SmallVector<ref<Expr>, 4> kids;
	for (unsigned i = cs->getNumOperands(); i != 0; --i) {
	  unsigned op = i-1;
	  ref<Expr> kid = evalConstant(cs->getOperand(op), ki);

	  uint64_t thisOffset = sl->getElementOffsetInBits(op),
	    nextOffset = (op == cs->getNumOperands() - 1)
	    ? sl->getSizeInBits()
	    : sl->getElementOffsetInBits(op+1);
	  if (nextOffset-thisOffset > kid->getWidth()) {
	    uint64_t paddingWidth = nextOffset-thisOffset-kid->getWidth();
	    kids.push_back(ConstantExpr::create(0, paddingWidth));
	  }

	  kids.push_back(kid);
	}
	ref<Expr> res = ConcatExpr::createN(kids.size(), kids.data());
	return cast<ConstantExpr>(res);
      } else if (const ConstantArray *ca = dyn_cast<ConstantArray>(c)){
	llvm::SmallVector<ref<Expr>, 4> kids;
	for (unsigned i = ca->getNumOperands(); i != 0; --i) {
	  unsigned op = i-1;
	  ref<Expr> kid = evalConstant(ca->getOperand(op), ki);
	  kids.push_back(kid);
	}
	ref<Expr> res = ConcatExpr::createN(kids.size(), kids.data());
	return cast<ConstantExpr>(res);
      } else if (const ConstantVector *cv = dyn_cast<ConstantVector>(c)) {
	llvm::SmallVector<ref<Expr>, 8> kids;
	const size_t numOperands = cv->getNumOperands();
	kids.reserve(numOperands);
	for (unsigned i = 0; i < numOperands; ++i) {
	  kids.push_back(evalConstant(cv->getOperand(i), ki));
	}
	ref<Expr> res = ConcatExpr::createN(numOperands, kids.data());
	assert(isa<ConstantExpr>(res) &&
	       "result of constant vector built is not a constant");
	return cast<ConstantExpr>(res);
      } else {
	std::string msg("Cannot handle constant ");
	llvm::raw_string_ostream os(msg);
	os << "'" << *c << "' at location "
	   << (ki ? ki->printFileLine().c_str() : "[unknown]");
	klee_error("%s", os.str().c_str());
      }
    }
  }

  ref<ConstantExpr> Executor::evalConstantExpr(const llvm::ConstantExpr *ce,
					       const KInstruction *ki) {
    llvm::Type *type = ce->getType();

    ref<ConstantExpr> op1(0), op2(0), op3(0);
    int numOperands = ce->getNumOperands();

    if (numOperands > 0) op1 = evalConstant(ce->getOperand(0), ki);
    if (numOperands > 1) op2 = evalConstant(ce->getOperand(1), ki);
    if (numOperands > 2) op3 = evalConstant(ce->getOperand(2), ki);

    std::string msg("Unknown ConstantExpr type");
    llvm::raw_string_ostream os(msg);

    switch (ce->getOpcode()) {
    default :
      os << "'" << *ce << "' at location "
	 << (ki ? ki->printFileLine().c_str() : "[unknown]");
      klee_error("%s", os.str().c_str());

    case Instruction::Trunc: 
      return op1->Extract(0, getWidthForLLVMType(type));
    case Instruction::ZExt:  return op1->ZExt(getWidthForLLVMType(type));
    case Instruction::SExt:  return op1->SExt(getWidthForLLVMType(type));
    case Instruction::Add:   return op1->Add(op2);
    case Instruction::Sub:   return op1->Sub(op2);
    case Instruction::Mul:   return op1->Mul(op2);
    case Instruction::SDiv:  return op1->SDiv(op2);
    case Instruction::UDiv:  return op1->UDiv(op2);
    case Instruction::SRem:  return op1->SRem(op2);
    case Instruction::URem:  return op1->URem(op2);
    case Instruction::And:   return op1->And(op2);
    case Instruction::Or:    return op1->Or(op2);
    case Instruction::Xor:   return op1->Xor(op2);
    case Instruction::Shl:   return op1->Shl(op2);
    case Instruction::LShr:  return op1->LShr(op2);
    case Instruction::AShr:  return op1->AShr(op2);
    case Instruction::BitCast:  return op1;

    case Instruction::IntToPtr:
      return op1->ZExt(getWidthForLLVMType(type));

    case Instruction::PtrToInt:
      return op1->ZExt(getWidthForLLVMType(type));

    case Instruction::GetElementPtr: {
      ref<ConstantExpr> base = op1->ZExt(Context::get().getPointerWidth());

      for (gep_type_iterator ii = gep_type_begin(ce), ie = gep_type_end(ce);
           ii != ie; ++ii) {
        ref<ConstantExpr> addend = 
          ConstantExpr::alloc(0, Context::get().getPointerWidth());

        if (StructType *st = dyn_cast<StructType>(*ii)) {
          const StructLayout *sl = kmodule->targetData->getStructLayout(st);
          const ConstantInt *ci = cast<ConstantInt>(ii.getOperand());

          addend = ConstantExpr::alloc(sl->getElementOffset((unsigned)
                                                            ci->getZExtValue()),
                                       Context::get().getPointerWidth());
        } else {
          const SequentialType *set = cast<SequentialType>(*ii);
          ref<ConstantExpr> index = 
            evalConstant(cast<Constant>(ii.getOperand()), ki);
          unsigned elementSize = 
            kmodule->targetData->getTypeStoreSize(set->getElementType());

          index = index->ZExt(Context::get().getPointerWidth());
          addend = index->Mul(ConstantExpr::alloc(elementSize, 
                                                  Context::get().getPointerWidth()));
        }

        base = base->Add(addend);
      }

      return base;
    }
      
    case Instruction::ICmp: {
      switch(ce->getPredicate()) {
      default: assert(0 && "unhandled ICmp predicate");
      case ICmpInst::ICMP_EQ:  return op1->Eq(op2);
      case ICmpInst::ICMP_NE:  return op1->Ne(op2);
      case ICmpInst::ICMP_UGT: return op1->Ugt(op2);
      case ICmpInst::ICMP_UGE: return op1->Uge(op2);
      case ICmpInst::ICMP_ULT: return op1->Ult(op2);
      case ICmpInst::ICMP_ULE: return op1->Ule(op2);
      case ICmpInst::ICMP_SGT: return op1->Sgt(op2);
      case ICmpInst::ICMP_SGE: return op1->Sge(op2);
      case ICmpInst::ICMP_SLT: return op1->Slt(op2);
      case ICmpInst::ICMP_SLE: return op1->Sle(op2);
      }
    }

    case Instruction::Select:
      return op1->isTrue() ? op2 : op3;

    case Instruction::FAdd:
    case Instruction::FSub:
    case Instruction::FMul:
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
    llvm_unreachable("Unsupported expression in evalConstantExpr");
    return op1;
  }
}
