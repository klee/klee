//===-- ExecutorUtil.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Executor.h"
#include "klee/Core/Context.h"

#include "klee/Config/Version.h"
#include "klee/Core/Interpreter.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KModule.h"
#include "klee/Solver/Solver.h"
#include "klee/Support/ErrorHandling.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <cassert>

using namespace llvm;

namespace klee {
extern llvm::cl::opt<unsigned> X86FPAsX87FP80;
ref<Expr> X87FP80ToFPTrunc(ref<Expr> arg, Expr::Width type,
                           llvm::APFloat::roundingMode rm) {
  ref<Expr> result = arg;
#ifdef ENABLE_FP
  Expr::Width resultType = type;
  if (Context::get().getPointerWidth() == 32 && arg->getWidth() == Expr::Fl80) {
    result = FPTruncExpr::create(arg, resultType, rm);
  }
#else
  klee_message(
      "You may enable x86-as-x87FP80 behaviour by passing the following options"
      " to cmake:\n"
      "\"-DENABLE_FLOATING_POINT=ON\"\n");
#endif // ENABLE_FP
  return result;
}

ref<Expr> FPToX87FP80Ext(ref<Expr> arg) {
  ref<Expr> result = arg;
#ifdef ENABLE_FP
  Expr::Width resultType = Expr::Fl80;
  if (Context::get().getPointerWidth() == 32) {
    result = FPExtExpr::create(arg, resultType);
  }
#else
  klee_message(
      "You may enable x86-as-x87FP80 behaviour by passing the following options"
      " to cmake:\n"
      "\"-DENABLE_FLOATING_POINT=ON\"\n");
#endif // ENABLE_FP
  return result;
}

ref<klee::Expr> Executor::evalConstant(const Constant *c,
                                       llvm::APFloat::roundingMode rm,
                                       const KInstruction *ki) {
  if (!ki) {
    KConstant *kc = kmodule->getKConstant(c);
    if (kc)
      ki = kc->ki;
  }

  if (const llvm::ConstantExpr *ce = dyn_cast<llvm::ConstantExpr>(c)) {
    return evalConstantExpr(ce, rm, ki);
  } else {
    if (const ConstantInt *ci = dyn_cast<ConstantInt>(c)) {
      return ConstantExpr::alloc(ci->getValue());
    } else if (const ConstantFP *cf = dyn_cast<ConstantFP>(c)) {
      ref<klee::ConstantExpr> result = ConstantExpr::alloc(cf->getValueAPF());
      if (X86FPAsX87FP80 && c->getType()->isFloatingPointTy() &&
          Context::get().getPointerWidth() == 32) {
        result = cast<klee::ConstantExpr>(FPToX87FP80Ext(result));
      }
      return result;
    } else if (const GlobalValue *gv = dyn_cast<GlobalValue>(c)) {
      auto it = globalAddresses.find(gv);
      assert(it != globalAddresses.end());
      return it->second;
    } else if (isa<ConstantPointerNull>(c)) {
      return Expr::createPointer(0);
    } else if (isa<UndefValue>(c) || isa<ConstantAggregateZero>(c)) {
      if (getWidthForLLVMType(c->getType()) == 0) {
        if (isa<llvm::LandingPadInst>(ki->inst())) {
          klee_warning_once(
              0, "Using zero size array fix for landingpad instruction filter");
          return ConstantExpr::create(0, 1);
        }
      }
      return ConstantExpr::create(0, getWidthForLLVMType(c->getType()));
    } else if (const ConstantDataSequential *cds =
                   dyn_cast<ConstantDataSequential>(c)) {
      // Handle a vector or array: first element has the smallest address,
      // the last element the highest
      std::vector<ref<Expr>> kids;
      for (unsigned i = cds->getNumElements(); i != 0; --i) {
        ref<Expr> kid = evalConstant(cds->getElementAsConstant(i - 1), rm, ki);
        kids.push_back(kid);
      }
      assert(Context::get().isLittleEndian() && "FIXME:Broken for big endian");
      ref<Expr> res = ConcatExpr::createN(kids.size(), kids.data());
      return cast<ConstantExpr>(res);
    } else if (const ConstantStruct *cs = dyn_cast<ConstantStruct>(c)) {
      const StructLayout *sl =
          kmodule->targetData->getStructLayout(cs->getType());
      llvm::SmallVector<ref<Expr>, 4> kids;
      for (unsigned i = cs->getNumOperands(); i != 0; --i) {
        unsigned op = i - 1;
        ref<Expr> kid = evalConstant(cs->getOperand(op), rm, ki);

        uint64_t thisOffset = sl->getElementOffsetInBits(op),
                 nextOffset = (op == cs->getNumOperands() - 1)
                                  ? sl->getSizeInBits()
                                  : sl->getElementOffsetInBits(op + 1);
        if (nextOffset - thisOffset > kid->getWidth()) {
          uint64_t paddingWidth = nextOffset - thisOffset - kid->getWidth();
          kids.push_back(ConstantExpr::create(0, paddingWidth));
        }

        kids.push_back(kid);
      }
      assert(Context::get().isLittleEndian() && "FIXME:Broken for big endian");
      ref<Expr> res = ConcatExpr::createN(kids.size(), kids.data());
      return cast<ConstantExpr>(res);
    } else if (const ConstantArray *ca = dyn_cast<ConstantArray>(c)) {
      llvm::SmallVector<ref<Expr>, 4> kids;
      for (unsigned i = ca->getNumOperands(); i != 0; --i) {
        unsigned op = i - 1;
        ref<Expr> kid = evalConstant(ca->getOperand(op), rm, ki);
        kids.push_back(kid);
      }
      assert(Context::get().isLittleEndian() && "FIXME:Broken for big endian");
      ref<Expr> res = ConcatExpr::createN(kids.size(), kids.data());
      return cast<ConstantExpr>(res);
    } else if (const ConstantVector *cv = dyn_cast<ConstantVector>(c)) {
      llvm::SmallVector<ref<Expr>, 8> kids;
      const size_t numOperands = cv->getNumOperands();
      kids.reserve(numOperands);
      for (unsigned i = numOperands; i != 0; --i) {
        kids.push_back(evalConstant(cv->getOperand(i - 1), rm, ki));
      }
      assert(Context::get().isLittleEndian() && "FIXME:Broken for big endian");
      ref<Expr> res = ConcatExpr::createN(numOperands, kids.data());
      assert(isa<ConstantExpr>(res) &&
             "result of constant vector built is not a constant");
      return cast<ConstantExpr>(res);
    } else if (const BlockAddress *ba = dyn_cast<BlockAddress>(c)) {
      // return the address of the specified basic block in the specified
      // function
      const auto arg_bb = (BasicBlock *)ba->getOperand(1);
      const auto res =
          Expr::createPointer(reinterpret_cast<std::uint64_t>(arg_bb));
      return cast<ConstantExpr>(res);
    } else {
      std::string msg("Cannot handle constant ");
      llvm::raw_string_ostream os(msg);
      os << "'" << *c << "' at location "
         << (ki ? ki->getSourceLocationString() : "[unknown]");
      klee_error("%s", os.str().c_str());
    }
  }
}

ref<klee::Expr> Executor::evaluateFCmp(unsigned int predicate,
                                       ref<klee::Expr> left,
                                       ref<klee::Expr> right) const {
  ref<klee::Expr> result = 0;
  switch (predicate) {
  case FCmpInst::FCMP_FALSE: {
    result = ConstantExpr::alloc(0, klee::Expr::Bool);
    break;
  }
  case FCmpInst::FCMP_OEQ: {
    result = FOEqExpr::create(left, right);
    break;
  }
  case FCmpInst::FCMP_OGT: {
    result = FOGtExpr::create(left, right);
    break;
  }
  case FCmpInst::FCMP_OGE: {
    result = FOGeExpr::create(left, right);
    break;
  }
  case FCmpInst::FCMP_OLT: {
    result = FOLtExpr::create(left, right);
    break;
  }
  case FCmpInst::FCMP_OLE: {
    result = FOLeExpr::create(left, right);
    break;
  }
  case FCmpInst::FCMP_ONE: {
    // This isn't NotExpr(FOEqExpr(arg))
    // because it is an ordered comparision and
    // should return false if either operand is NaN.
    //
    // ¬(isnan(l) ∨ isnan(r)) ∧ ¬(foeq(l, r))
    //
    //  ===
    //
    // ¬( (isnan(l) ∨ isnan(r)) ∨ foeq(l,r))
    result = NotExpr::create(OrExpr::create(IsNaNExpr::either(left, right),
                                            FOEqExpr::create(left, right)));
    break;
  }
  case FCmpInst::FCMP_ORD: {
    result = NotExpr::create(IsNaNExpr::either(left, right));
    break;
  }
  case FCmpInst::FCMP_UNO: {
    result = IsNaNExpr::either(left, right);
    break;
  }
  case FCmpInst::FCMP_UEQ: {
    result = OrExpr::create(IsNaNExpr::either(left, right),
                            FOEqExpr::create(left, right));
    break;
  }
  case FCmpInst::FCMP_UGT: {
    result = OrExpr::create(IsNaNExpr::either(left, right),
                            FOGtExpr::create(left, right));
    break;
  }
  case FCmpInst::FCMP_UGE: {
    result = OrExpr::create(IsNaNExpr::either(left, right),
                            FOGeExpr::create(left, right));
    break;
  }
  case FCmpInst::FCMP_ULT: {
    result = OrExpr::create(IsNaNExpr::either(left, right),
                            FOLtExpr::create(left, right));
    break;
  }
  case FCmpInst::FCMP_ULE: {
    result = OrExpr::create(IsNaNExpr::either(left, right),
                            FOLeExpr::create(left, right));
    break;
  }
  case FCmpInst::FCMP_UNE: {
    // Unordered comparision so should
    // return true if either arg is NaN.
    // If either arg to ``FOEqExpr::create()``
    // is a NaN then the result is false which gets
    // negated giving us true when either arg to the instruction
    // is a NaN.
    result = NotExpr::create(FOEqExpr::create(left, right));
    break;
  }
  case FCmpInst::FCMP_TRUE: {
    result = ConstantExpr::alloc(1, Expr::Bool);
    break;
  }
  default:
    llvm_unreachable("Unhandled FCmp predicate");
  }
  return result;
}

ref<Expr> Executor::evalConstantExpr(const llvm::ConstantExpr *ce,
                                     llvm::APFloat::roundingMode rm,
                                     const KInstruction *ki) {
  llvm::Type *type = ce->getType();

  ref<Expr> op1(0), op2(0), op3(0);
  int numOperands = ce->getNumOperands();

  if (numOperands > 0)
    op1 = evalConstant(ce->getOperand(0), rm, ki);
  if (numOperands > 1)
    op2 = evalConstant(ce->getOperand(1), rm, ki);
  if (numOperands > 2)
    op3 = evalConstant(ce->getOperand(2), rm, ki);

  /* Checking for possible errors during constant folding */
  switch (ce->getOpcode()) {
  case Instruction::SDiv:
  case Instruction::UDiv:
  case Instruction::SRem:
  case Instruction::URem: {
    if (auto op2Const = dyn_cast<ConstantExpr>(op2)) {
      if (op2Const->getLimitedValue() == 0) {
        std::string msg(
            "Division/modulo by zero during constant folding at location ");
        llvm::raw_string_ostream os(msg);
        os << (ki ? ki->getSourceLocationString() : "[unknown]");
        klee_error("%s", os.str().c_str());
      }
    }
    break;
  }
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
    if (auto op2Const = dyn_cast<ConstantExpr>(op2)) {
      if (op2Const->getLimitedValue() >= op1->getWidth()) {
        std::string msg("Overshift during constant folding at location ");
        llvm::raw_string_ostream os(msg);
        os << (ki ? ki->getSourceLocationString() : "[unknown]");
        klee_error("%s", os.str().c_str());
      }
    }
  }

  std::string msg("Unknown ConstantExpr type");
  llvm::raw_string_ostream os(msg);

  switch (ce->getOpcode()) {
  default:
    os << "'" << *ce << "' at location "
       << (ki ? ki->getSourceLocationString() : "[unknown]");
    klee_error("%s", os.str().c_str());

  case Instruction::Trunc:
    return ExtractExpr::create(op1, 0, getWidthForLLVMType(type));
  case Instruction::ZExt:
    return ZExtExpr::create(op1, getWidthForLLVMType(type));
  case Instruction::SExt:
    return SExtExpr::create(op1, getWidthForLLVMType(type));
  case Instruction::Add:
    return AddExpr::create(op1, op2);
  case Instruction::Sub:
    return SubExpr::create(op1, op2);
  case Instruction::Mul:
    return MulExpr::create(op1, op2);
  case Instruction::SDiv:
    return SDivExpr::create(op1, op2);
  case Instruction::UDiv:
    return UDivExpr::create(op1, op2);
  case Instruction::SRem:
    return SRemExpr::create(op1, op2);
  case Instruction::URem:
    return URemExpr::create(op1, op2);
  case Instruction::And:
    return AndExpr::create(op1, op2);
  case Instruction::Or:
    return OrExpr::create(op1, op2);
  case Instruction::Xor:
    return XorExpr::create(op1, op2);
  case Instruction::Shl:
    return ShlExpr::create(op1, op2);
  case Instruction::LShr:
    return LShrExpr::create(op1, op2);
  case Instruction::AShr:
    return AShrExpr::create(op1, op2);
  case Instruction::BitCast: {
    ref<klee::Expr> result = op1;

    if (X86FPAsX87FP80 && result->getWidth() == Expr::Fl80 &&
        !ce->getType()->isFloatingPointTy() &&
        Context::get().getPointerWidth() == 32) {
      result = cast<klee::ConstantExpr>(
          X87FP80ToFPTrunc(result, getWidthForLLVMType(type), rm));
    }

    if (X86FPAsX87FP80 && ce->getType()->isFloatingPointTy() &&
        Context::get().getPointerWidth() == 32) {
      result = cast<klee::ConstantExpr>(FPToX87FP80Ext(result));
    }

    if (constantGepExprBases.count(result)) {
      constantGepExprBases[result] = {constantGepExprBases[result].first,
                                      ce->getType()};
    }

    return result;
  }

  case Instruction::IntToPtr:
    return ZExtExpr::create(op1, getWidthForLLVMType(type));

  case Instruction::PtrToInt:
    return ZExtExpr::create(op1, getWidthForLLVMType(type));

  case Instruction::GetElementPtr: {
    ref<Expr> base = ZExtExpr::create(op1, Context::get().getPointerWidth());
    for (gep_type_iterator ii = gep_type_begin(ce), ie = gep_type_end(ce);
         ii != ie; ++ii) {
      ref<Expr> indexOp = evalConstant(cast<Constant>(ii.getOperand()), rm, ki);
      if (indexOp->isZero())
        continue;

      // Handle a struct index, which adds its field offset to the pointer.
      if (auto STy = ii.getStructTypeOrNull()) {
        auto indexOpConst = dyn_cast<ConstantExpr>(indexOp);
        if (!indexOpConst) {
          klee_error(
              "Found non-constant index in evaluation of llvm::ConstantExpr");
        }
        unsigned ElementIdx = indexOpConst->getZExtValue();
        const StructLayout *SL = kmodule->targetData->getStructLayout(STy);
        base = AddExpr::create(
            base, ConstantExpr::alloc(APInt(Context::get().getPointerWidth(),
                                            SL->getElementOffset(ElementIdx))));
        continue;
      }

      // For array or vector indices, scale the index by the size of the type.
      // Indices can be negative
      base = AddExpr::create(
          base,
          MulExpr::create(
              SExtExpr::create(indexOp, Context::get().getPointerWidth()),
              ConstantExpr::alloc(APInt(Context::get().getPointerWidth(),
                                        kmodule->targetData->getTypeAllocSize(
                                            ii.getIndexedType())))));
    }

    if (constantGepExprBases.count(op1)) {
      constantGepExprBases[base] = constantGepExprBases[op1];
    } else {
      constantGepExprBases[base] = {
          op1, llvm::cast<GEPOperator>(ce)->getSourceElementType()};
    }

    return base;
  }

  case Instruction::ICmp: {
    switch (ce->getPredicate()) {
    default:
      assert(0 && "unhandled ICmp predicate");
    case ICmpInst::ICMP_EQ:
      return EqExpr::create(op1, op2);
    case ICmpInst::ICMP_NE:
      return NeExpr::create(op1, op2);
    case ICmpInst::ICMP_UGT:
      return UgtExpr::create(op1, op2);
    case ICmpInst::ICMP_UGE:
      return UgeExpr::create(op1, op2);
    case ICmpInst::ICMP_ULT:
      return UltExpr::create(op1, op2);
    case ICmpInst::ICMP_ULE:
      return UleExpr::create(op1, op2);
    case ICmpInst::ICMP_SGT:
      return SgtExpr::create(op1, op2);
    case ICmpInst::ICMP_SGE:
      return SgeExpr::create(op1, op2);
    case ICmpInst::ICMP_SLT:
      return SltExpr::create(op1, op2);
    case ICmpInst::ICMP_SLE:
      return SleExpr::create(op1, op2);
    }
  }

  case Instruction::Select:
    return op1->isTrue() ? op2 : op3;

  case Instruction::FAdd:
    return FAddExpr::create(op1, op2, rm);
  case Instruction::FSub:
    return FSubExpr::create(op1, op2, rm);
  case Instruction::FMul:
    return FMulExpr::create(op1, op2, rm);
  case Instruction::FDiv:
    return FDivExpr::create(op1, op2, rm);
  case Instruction::FRem: {
    return FRemExpr::create(op1, op2, rm);
  }

  case Instruction::FPTrunc: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return FPTruncExpr::create(op1, width, rm);
  }
  case Instruction::FPExt: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return FPExtExpr::create(op1, width);
  }
  case Instruction::UIToFP: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return UIToFPExpr::create(op1, width, rm);
  }
  case Instruction::SIToFP: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return SIToFPExpr::create(op1, width, rm);
  }
  case Instruction::FPToUI: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return FPToUIExpr::create(op1, width, rm);
  }
  case Instruction::FPToSI: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return FPToSIExpr::create(op1, width, rm);
  }
  case Instruction::FCmp: {
    return evaluateFCmp(ce->getPredicate(), op1, op2);
  }
    assert(0 && "floating point ConstantExprs unsupported");
  }
  llvm_unreachable("Unsupported expression in evalConstantExpr");
  return op1;
}
} // namespace klee
