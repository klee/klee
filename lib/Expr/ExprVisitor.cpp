//===-- ExprVisitor.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/ExprVisitor.h"

#include "klee/Expr/Expr.h"

#include "llvm/Support/CommandLine.h"

namespace {
llvm::cl::opt<bool> UseVisitorHash(
    "use-visitor-hash",
    llvm::cl::desc(
        "Use hash-consing during expression visitation (default=true)"),
    llvm::cl::init(true), llvm::cl::cat(klee::ExprCat));
}

using namespace klee;

ref<Expr> ExprVisitor::visit(const ref<Expr> &e) {
  if (!UseVisitorHash || isa<ConstantExpr>(e)) {
    return visitActual(e);
  } else {
    auto result = visited.get(e);

    if (result.second) {
      return result.first;
    } else {
      ref<Expr> res = visitActual(e);
      visited.add({e, res});
      return res;
    }
  }
}

ref<Expr> ExprVisitor::visitActual(const ref<Expr> &e) {
  if (isa<ConstantExpr>(e)) {
    return e;
  } else {
    Expr &ep = *e.get();

    Action res = visitExpr(ep);
    switch (res.kind) {
    case Action::DoChildren:
      // continue with normal action
      break;
    case Action::SkipChildren:
      return e;
    case Action::ChangeTo:
      return res.argument;
    }

    switch (ep.getKind()) {
    case Expr::NotOptimized:
      res = visitNotOptimized(static_cast<NotOptimizedExpr &>(ep));
      break;
    case Expr::Read:
      res = visitRead(static_cast<ReadExpr &>(ep));
      break;
    case Expr::Select:
      res = visitSelect(static_cast<SelectExpr &>(ep));
      break;
    case Expr::Concat:
      res = visitConcat(static_cast<ConcatExpr &>(ep));
      break;
    case Expr::Extract:
      res = visitExtract(static_cast<ExtractExpr &>(ep));
      break;
    case Expr::ZExt:
      res = visitZExt(static_cast<ZExtExpr &>(ep));
      break;
    case Expr::SExt:
      res = visitSExt(static_cast<SExtExpr &>(ep));
      break;
    case Expr::FPExt:
      res = visitFPExt(static_cast<FPExtExpr &>(ep));
      break;
    case Expr::FPTrunc:
      res = visitFPTrunc(static_cast<FPTruncExpr &>(ep));
      break;
    case Expr::FPToUI:
      res = visitFPToUI(static_cast<FPToUIExpr &>(ep));
      break;
    case Expr::FPToSI:
      res = visitFPToSI(static_cast<FPToSIExpr &>(ep));
      break;
    case Expr::UIToFP:
      res = visitUIToFP(static_cast<UIToFPExpr &>(ep));
      break;
    case Expr::SIToFP:
      res = visitSIToFP(static_cast<SIToFPExpr &>(ep));
      break;
    case Expr::Add:
      res = visitAdd(static_cast<AddExpr &>(ep));
      break;
    case Expr::Sub:
      res = visitSub(static_cast<SubExpr &>(ep));
      break;
    case Expr::Mul:
      res = visitMul(static_cast<MulExpr &>(ep));
      break;
    case Expr::UDiv:
      res = visitUDiv(static_cast<UDivExpr &>(ep));
      break;
    case Expr::SDiv:
      res = visitSDiv(static_cast<SDivExpr &>(ep));
      break;
    case Expr::URem:
      res = visitURem(static_cast<URemExpr &>(ep));
      break;
    case Expr::SRem:
      res = visitSRem(static_cast<SRemExpr &>(ep));
      break;
    case Expr::Not:
      res = visitNot(static_cast<NotExpr &>(ep));
      break;
    case Expr::And:
      res = visitAnd(static_cast<AndExpr &>(ep));
      break;
    case Expr::Or:
      res = visitOr(static_cast<OrExpr &>(ep));
      break;
    case Expr::Xor:
      res = visitXor(static_cast<XorExpr &>(ep));
      break;
    case Expr::Shl:
      res = visitShl(static_cast<ShlExpr &>(ep));
      break;
    case Expr::LShr:
      res = visitLShr(static_cast<LShrExpr &>(ep));
      break;
    case Expr::AShr:
      res = visitAShr(static_cast<AShrExpr &>(ep));
      break;
    case Expr::Eq:
      res = visitEq(static_cast<EqExpr &>(ep));
      break;
    case Expr::Ne:
      res = visitNe(static_cast<NeExpr &>(ep));
      break;
    case Expr::Ult:
      res = visitUlt(static_cast<UltExpr &>(ep));
      break;
    case Expr::Ule:
      res = visitUle(static_cast<UleExpr &>(ep));
      break;
    case Expr::Ugt:
      res = visitUgt(static_cast<UgtExpr &>(ep));
      break;
    case Expr::Uge:
      res = visitUge(static_cast<UgeExpr &>(ep));
      break;
    case Expr::Slt:
      res = visitSlt(static_cast<SltExpr &>(ep));
      break;
    case Expr::Sle:
      res = visitSle(static_cast<SleExpr &>(ep));
      break;
    case Expr::Sgt:
      res = visitSgt(static_cast<SgtExpr &>(ep));
      break;
    case Expr::Sge:
      res = visitSge(static_cast<SgeExpr &>(ep));
      break;
    case Expr::FOEq:
      res = visitFOEq(static_cast<FOEqExpr &>(ep));
      break;
    case Expr::FOLt:
      res = visitFOLt(static_cast<FOLtExpr &>(ep));
      break;
    case Expr::FOLe:
      res = visitFOLe(static_cast<FOLeExpr &>(ep));
      break;
    case Expr::FOGt:
      res = visitFOGt(static_cast<FOGtExpr &>(ep));
      break;
    case Expr::FOGe:
      res = visitFOGe(static_cast<FOGeExpr &>(ep));
      break;
    case Expr::IsNaN:
      res = visitIsNaN(static_cast<IsNaNExpr &>(ep));
      break;
    case Expr::IsInfinite:
      res = visitIsInfinite(static_cast<IsInfiniteExpr &>(ep));
      break;
    case Expr::IsNormal:
      res = visitIsNormal(static_cast<IsNormalExpr &>(ep));
      break;
    case Expr::IsSubnormal:
      res = visitIsSubnormal(static_cast<IsSubnormalExpr &>(ep));
      break;
    case Expr::FAdd:
      res = visitFAdd(static_cast<FAddExpr &>(ep));
      break;
    case Expr::FSub:
      res = visitFSub(static_cast<FSubExpr &>(ep));
      break;
    case Expr::FMul:
      res = visitFMul(static_cast<FMulExpr &>(ep));
      break;
    case Expr::FDiv:
      res = visitFDiv(static_cast<FDivExpr &>(ep));
      break;
    case Expr::FRem:
      res = visitFRem(static_cast<FRemExpr &>(ep));
      break;
    case Expr::FMax:
      res = visitFMax(static_cast<FMaxExpr &>(ep));
      break;
    case Expr::FMin:
      res = visitFMin(static_cast<FMinExpr &>(ep));
      break;
    case Expr::FSqrt:
      res = visitFSqrt(static_cast<FSqrtExpr &>(ep));
      break;
    case Expr::FRint:
      res = visitFRint(static_cast<FRintExpr &>(ep));
      break;
    case Expr::FAbs:
      res = visitFAbs(static_cast<FAbsExpr &>(ep));
      break;
    case Expr::FNeg:
      res = visitFNeg(static_cast<FNegExpr &>(ep));
      break;
    case Expr::Pointer:
      res = visitPointer(static_cast<PointerExpr &>(ep));
      break;
    case Expr::ConstantPointer:
      res = visitConstantPointer(static_cast<ConstantPointerExpr &>(ep));
      break;
    case Expr::Constant:
    default:
      assert(0 && "invalid expression kind");
    }

    switch (res.kind) {
    default:
      assert(0 && "invalid kind");
    case Action::DoChildren: {
      bool rebuild = false;
      ref<Expr> e(&ep), kids[8];
      unsigned count = ep.getNumKids();
      for (unsigned i = 0; i < count; i++) {
        ref<Expr> kid = ep.getKid(i);
        kids[i] = visit(kid);
        if (kids[i] != kid)
          rebuild = true;
      }
      if (rebuild) {
        e = ep.rebuild(kids);
        if (recursive)
          e = visit(e);
      }
      if (!isa<ConstantExpr>(e)) {
        res = visitExprPost(*e.get());
        if (res.kind == Action::ChangeTo)
          e = res.argument;
      }
      return e;
    }
    case Action::SkipChildren:
      return e;
    case Action::ChangeTo:
      return res.argument;
    }
  }
}

ExprVisitor::Action ExprVisitor::visitExpr(const Expr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitExprPost(const Expr &) {
  return Action::skipChildren();
}

ExprVisitor::Action ExprVisitor::visitNotOptimized(const NotOptimizedExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitRead(const ReadExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSelect(const SelectExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitConcat(const ConcatExpr &) {
  return Action::doChildren();
}
ExprVisitor::Action ExprVisitor::visitExtract(const ExtractExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitZExt(const ZExtExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSExt(const SExtExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFPExt(const FPExtExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFPTrunc(const FPTruncExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFPToUI(const FPToUIExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFPToSI(const FPToSIExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitUIToFP(const UIToFPExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSIToFP(const SIToFPExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFSqrt(const FSqrtExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFRint(const FRintExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFAbs(const FAbsExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFNeg(const FNegExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitAdd(const AddExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSub(const SubExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitMul(const MulExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitUDiv(const UDivExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSDiv(const SDivExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitURem(const URemExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSRem(const SRemExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitNot(const NotExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitAnd(const AndExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitOr(const OrExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitXor(const XorExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitShl(const ShlExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitLShr(const LShrExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitAShr(const AShrExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitEq(const EqExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitNe(const NeExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitUlt(const UltExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitUle(const UleExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitUgt(const UgtExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitUge(const UgeExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSlt(const SltExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSle(const SleExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSgt(const SgtExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSge(const SgeExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOEq(const FOEqExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOLt(const FOLtExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOLe(const FOLeExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOGt(const FOGtExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOGe(const FOGeExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitIsNaN(const IsNaNExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitIsInfinite(const IsInfiniteExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitIsNormal(const IsNormalExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitIsSubnormal(const IsSubnormalExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFAdd(const FAddExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFSub(const FSubExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFMul(const FMulExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFDiv(const FDivExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFRem(const FRemExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFMax(const FMaxExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFMin(const FMinExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitPointer(const PointerExpr &) {
  return Action::doChildren();
}

ExprVisitor::Action
ExprVisitor::visitConstantPointer(const ConstantPointerExpr &) {
  return Action::doChildren();
}
