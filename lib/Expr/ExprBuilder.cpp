//===-- ExprBuilder.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/ExprBuilder.h"

using namespace klee;

namespace {
  class DefaultExprBuilder : public ExprBuilder {
    virtual ref<Expr> Constant(uint64_t Value, Expr::Width W) {
      return ConstantExpr::alloc(Value, W);
    }

    virtual ref<Expr> NotOptimized(const ref<Expr> &Index) {
      return NotOptimizedExpr::alloc(Index);
    }

    virtual ref<Expr> Read(const UpdateList &Updates,
                           const ref<Expr> &Index) {
      return ReadExpr::alloc(Updates, Index);
    }

    virtual ref<Expr> Select(const ref<Expr> &Cond,
                             const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SelectExpr::alloc(Cond, LHS, RHS);
    }

    virtual ref<Expr> Concat(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return ConcatExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Extract(const ref<Expr> &LHS,
                              unsigned Offset, Expr::Width W) {
      return ExtractExpr::alloc(LHS, Offset, W);
    }

    virtual ref<Expr> ZExt(const ref<Expr> &LHS, Expr::Width W) {
      return ZExtExpr::alloc(LHS, W);
    }

    virtual ref<Expr> SExt(const ref<Expr> &LHS, Expr::Width W) {
      return SExtExpr::alloc(LHS, W);
    }

    virtual ref<Expr> Add(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return AddExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Sub(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SubExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Mul(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return MulExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> UDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UDivExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> SDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SDivExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> URem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return URemExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> SRem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SRemExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> And(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return AndExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Or(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return OrExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Xor(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return XorExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Shl(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return ShlExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> LShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return LShrExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> AShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return AShrExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Eq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return EqExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Ne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return NeExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Ult(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UltExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Ule(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UleExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Ugt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UgtExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Uge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UgeExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Slt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SltExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Sle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SleExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Sgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SgtExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Sge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SgeExpr::alloc(LHS, RHS);
    }
  };

  class ConstantFoldingExprBuilder : public ExprBuilder {
    ExprBuilder *Base;

  public:
    ConstantFoldingExprBuilder(ExprBuilder *_Base) : Base(_Base) {}
    ~ConstantFoldingExprBuilder() { 
      delete Base;
    }

    virtual ref<Expr> Constant(uint64_t Value, Expr::Width W) {
      return Base->Constant(Value, W);
    }

    virtual ref<Expr> NotOptimized(const ref<Expr> &Index) {
      return Base->NotOptimized(Index);
    }

    virtual ref<Expr> Read(const UpdateList &Updates,
                           const ref<Expr> &Index) {
      // Roll back through writes when possible.
      const UpdateNode *UN = Updates.head;
      while (UN && Eq(Index, UN->index)->isFalse())
        UN = UN->next;
      
      return Base->Read(UpdateList(Updates.root, UN), Index);
    }

    virtual ref<Expr> Select(const ref<Expr> &Cond,
                             const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Cond))
        return CE->isTrue() ? LHS : RHS;

      return Base->Select(Cond, LHS, RHS);
    }

    virtual ref<Expr> Concat(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Concat(RCE);

      return Base->Concat(LHS, RHS);
    }

    virtual ref<Expr> Extract(const ref<Expr> &LHS,
                              unsigned Offset, Expr::Width W) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LHS))
        return CE->Extract(Offset, W);

      return Base->Extract(LHS, Offset, W);
    }

    virtual ref<Expr> ZExt(const ref<Expr> &LHS, Expr::Width W) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LHS))
        return CE->ZExt(W);

      return Base->ZExt(LHS, W);
    }

    virtual ref<Expr> SExt(const ref<Expr> &LHS, Expr::Width W) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LHS))
        return CE->SExt(W);

      return Base->SExt(LHS, W);
    }

    virtual ref<Expr> Add(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Add(RCE);

      return Base->Add(LHS, RHS);
    }

    virtual ref<Expr> Sub(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Sub(RCE);

      return Base->Sub(LHS, RHS);
    }

    virtual ref<Expr> Mul(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Mul(RCE);

      return Base->Mul(LHS, RHS);
    }

    virtual ref<Expr> UDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->UDiv(RCE);

      return Base->UDiv(LHS, RHS);
    }

    virtual ref<Expr> SDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->SDiv(RCE);

      return Base->SDiv(LHS, RHS);
    }

    virtual ref<Expr> URem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->URem(RCE);

      return Base->URem(LHS, RHS);
    }

    virtual ref<Expr> SRem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->SRem(RCE);

      return Base->SRem(LHS, RHS);
    }

    virtual ref<Expr> And(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->And(RCE);

      return Base->And(LHS, RHS);
    }

    virtual ref<Expr> Or(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Or(RCE);

      return Base->Or(LHS, RHS);
    }

    virtual ref<Expr> Xor(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Xor(RCE);

      return Base->Xor(LHS, RHS);
    }

    virtual ref<Expr> Shl(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Shl(RCE);

      return Base->Shl(LHS, RHS);
    }

    virtual ref<Expr> LShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->LShr(RCE);

      return Base->LShr(LHS, RHS);
    }

    virtual ref<Expr> AShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->AShr(RCE);

      return Base->AShr(LHS, RHS);
    }

    virtual ref<Expr> Eq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Eq(RCE);

      return Base->Eq(LHS, RHS);
    }

    virtual ref<Expr> Ne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Ne(RCE);

      return Base->Ne(LHS, RHS);
    }

    virtual ref<Expr> Ult(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Ult(RCE);

      return Base->Ult(LHS, RHS);
    }

    virtual ref<Expr> Ule(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Ule(RCE);

      return Base->Ule(LHS, RHS);
    }

    virtual ref<Expr> Ugt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Ugt(RCE);

      return Base->Ugt(LHS, RHS);
    }

    virtual ref<Expr> Uge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Uge(RCE);

      return Base->Uge(LHS, RHS);
    }

    virtual ref<Expr> Slt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Slt(RCE);

      return Base->Slt(LHS, RHS);
    }

    virtual ref<Expr> Sle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Sle(RCE);

      return Base->Sle(LHS, RHS);
    }

    virtual ref<Expr> Sgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Sgt(RCE);

      return Base->Sgt(LHS, RHS);
    }

    virtual ref<Expr> Sge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS))
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Sge(RCE);

      return Base->Sge(LHS, RHS);
    }
  };

  class FoldingExprBuilder : public ExprBuilder {
    ExprBuilder *Base;

  public:
    FoldingExprBuilder(ExprBuilder *_Base) : Base(_Base) {}
    ~FoldingExprBuilder() { 
      delete Base;
    }

    virtual ref<Expr> Constant(uint64_t Value, Expr::Width W) {
      return Base->Constant(Value, W);
    }

    virtual ref<Expr> NotOptimized(const ref<Expr> &Index) {
      return Base->NotOptimized(Index);
    }

    virtual ref<Expr> Read(const UpdateList &Updates,
                           const ref<Expr> &Index) {
      return Base->Read(Updates, Index);
    }

    virtual ref<Expr> Select(const ref<Expr> &Cond,
                             const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Select(Cond, LHS, RHS);
    }

    virtual ref<Expr> Concat(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Concat(LHS, RHS);
    }

    virtual ref<Expr> Extract(const ref<Expr> &LHS,
                              unsigned Offset, Expr::Width W) {
      return Base->Extract(LHS, Offset, W);
    }

    virtual ref<Expr> ZExt(const ref<Expr> &LHS, Expr::Width W) {
      return Base->ZExt(LHS, W);
    }

    virtual ref<Expr> SExt(const ref<Expr> &LHS, Expr::Width W) {
      return Base->SExt(LHS, W);
    }

    virtual ref<Expr> Add(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Add(LHS, RHS);
    }

    virtual ref<Expr> Sub(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Sub(LHS, RHS);
    }

    virtual ref<Expr> Mul(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Mul(LHS, RHS);
    }

    virtual ref<Expr> UDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->UDiv(LHS, RHS);
    }

    virtual ref<Expr> SDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->SDiv(LHS, RHS);
    }

    virtual ref<Expr> URem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->URem(LHS, RHS);
    }

    virtual ref<Expr> SRem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->SRem(LHS, RHS);
    }

    virtual ref<Expr> And(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->And(LHS, RHS);
    }

    virtual ref<Expr> Or(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Or(LHS, RHS);
    }

    virtual ref<Expr> Xor(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Xor(LHS, RHS);
    }

    virtual ref<Expr> Shl(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Shl(LHS, RHS);
    }

    virtual ref<Expr> LShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->LShr(LHS, RHS);
    }

    virtual ref<Expr> AShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->AShr(LHS, RHS);
    }

    virtual ref<Expr> Eq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Eq(LHS, RHS);
    }

    virtual ref<Expr> Ne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Ne(LHS, RHS);
    }

    virtual ref<Expr> Ult(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Ult(LHS, RHS);
    }

    virtual ref<Expr> Ule(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Ule(LHS, RHS);
    }

    virtual ref<Expr> Ugt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Ugt(LHS, RHS);
    }

    virtual ref<Expr> Uge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Uge(LHS, RHS);
    }

    virtual ref<Expr> Slt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Slt(LHS, RHS);
    }

    virtual ref<Expr> Sle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Sle(LHS, RHS);
    }

    virtual ref<Expr> Sgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Sgt(LHS, RHS);
    }

    virtual ref<Expr> Sge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Sge(LHS, RHS);
    }
  };
}

ExprBuilder *klee::createDefaultExprBuilder() {
  return new DefaultExprBuilder();
}

ExprBuilder *klee::createConstantFoldingExprBuilder(ExprBuilder *Base) {
  return new ConstantFoldingExprBuilder(Base);
}

ExprBuilder *klee::createFoldingExprBuilder(ExprBuilder *Base) {
  return new FoldingExprBuilder(Base);
}
