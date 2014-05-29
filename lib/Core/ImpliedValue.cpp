//===-- ImpliedValue.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ImpliedValue.h"

#include "Context.h"
#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/Solver.h"
// FIXME: Use APInt.
#include "klee/Internal/Support/IntEvaluation.h"

#include "klee/util/ExprUtil.h"

#include <map>
#include <set>

using namespace klee;

// XXX we really want to do some sort of canonicalization of exprs
// globally so that cases below become simpler
void ImpliedValue::getImpliedValues(ref<Expr> e,
                                    ref<ConstantExpr> value,
                                    ImpliedValueList &results) {
  switch (e->getKind()) {
  case Expr::Constant: {
    assert(value == cast<ConstantExpr>(e) && 
           "error in implied value calculation");
    break;
  }

    // Special

  case Expr::NotOptimized: break;

  case Expr::Read: {
    // XXX in theory it is possible to descend into a symbolic index
    // under certain circumstances (all values known, known value
    // unique, or range known, max / min hit). Seems unlikely this
    // would work often enough to be worth the effort.
    ReadExpr *re = cast<ReadExpr>(e);
    results.push_back(std::make_pair(re, value));
    break;
  }
    
  case Expr::Select: {
    // not much to do, could improve with range analysis
    SelectExpr *se = cast<SelectExpr>(e);
    
    if (ConstantExpr *TrueCE = dyn_cast<ConstantExpr>(se->trueExpr)) {
      if (ConstantExpr *FalseCE = dyn_cast<ConstantExpr>(se->falseExpr)) {
        if (TrueCE != FalseCE) {
          if (value == TrueCE) {
            getImpliedValues(se->cond, ConstantExpr::alloc(1, Expr::Bool), 
                             results);
          } else {
            assert(value == FalseCE &&
                   "err in implied value calculation");
            getImpliedValues(se->cond, ConstantExpr::alloc(0, Expr::Bool), 
                             results);
          }
        }
      }
    }
    break;
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    getImpliedValues(ce->getKid(0), value->Extract(ce->getKid(1)->getWidth(),
                                                   ce->getKid(0)->getWidth()),
                     results);
    getImpliedValues(ce->getKid(1), value->Extract(0,
                                                   ce->getKid(1)->getWidth()),
                     results);
    break;
  }
    
  case Expr::Extract: {
    // XXX, could do more here with "some bits" mask
    break;
  }

    // Casting

  case Expr::ZExt: 
  case Expr::SExt: {
    CastExpr *ce = cast<CastExpr>(e);
    getImpliedValues(ce->src, value->Extract(0, ce->src->getWidth()),
                     results);
    break;
  }

    // Arithmetic

  case Expr::Add: { // constants on left
    BinaryExpr *be = cast<BinaryExpr>(e);
    // C_0 + A = C  =>  A = C - C_0
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left))
      getImpliedValues(be->right, value->Sub(CE), results);
    break;
  }
  case Expr::Sub: { // constants on left
    BinaryExpr *be = cast<BinaryExpr>(e);
    // C_0 - A = C  =>  A = C_0 - C
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left))
      getImpliedValues(be->right, CE->Sub(value), results);
    break;
  }
  case Expr::Mul: {
    // FIXME: Can do stuff here, but need valid mask and other things because of
    // bits that might be lost.
    break;
  }

  case Expr::UDiv:
  case Expr::SDiv:
  case Expr::URem:
  case Expr::SRem:
    break;

    // Binary

  case Expr::And: {
    BinaryExpr *be = cast<BinaryExpr>(e);
    if (be->getWidth() == Expr::Bool) {
      if (value->isTrue()) {
        getImpliedValues(be->left, value, results);
        getImpliedValues(be->right, value, results);
      }
    } else {
      // FIXME; We can propogate a mask here where we know "some bits". May or
      // may not be useful.
    }
    break;
  }
  case Expr::Or: {
    BinaryExpr *be = cast<BinaryExpr>(e);
    if (value->isZero()) {
      getImpliedValues(be->left, 0, results);
      getImpliedValues(be->right, 0, results);
    } else {
      // FIXME: Can do more?
    }
    break;
  }
  case Expr::Xor: {
    BinaryExpr *be = cast<BinaryExpr>(e);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left))
      getImpliedValues(be->right, value->Xor(CE), results);
    break;
  }

    // Comparison
  case Expr::Ne: 
    value = value->Not();
    /* fallthru */
  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    if (value->isTrue()) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ee->left))
        getImpliedValues(ee->right, CE, results);
    } else {
      // Look for limited value range.
      //
      // In general anytime one side was restricted to two values we can apply
      // this trick. The only obvious case where this occurs, aside from
      // booleans, is as the result of a select expression where the true and
      // false branches are single valued and distinct.
      
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ee->left))
        if (CE->getWidth() == Expr::Bool)
          getImpliedValues(ee->right, CE->Not(), results);
    }
    break;
  }
    
  default:
    break;
  }
}
    
void ImpliedValue::checkForImpliedValues(Solver *S, ref<Expr> e, 
                                         ref<ConstantExpr> value) {
  std::vector<ref<ReadExpr> > reads;
  std::map<ref<ReadExpr>, ref<ConstantExpr> > found;
  ImpliedValueList results;

  getImpliedValues(e, value, results);

  for (ImpliedValueList::iterator i = results.begin(), ie = results.end();
       i != ie; ++i) {
    std::map<ref<ReadExpr>, ref<ConstantExpr> >::iterator it = 
      found.find(i->first);
    if (it != found.end()) {
      assert(it->second == i->second && "Invalid ImpliedValue!");
    } else {
      found.insert(std::make_pair(i->first, i->second));
    }
  }

  findReads(e, false, reads);
  std::set< ref<ReadExpr> > readsSet(reads.begin(), reads.end());
  reads = std::vector< ref<ReadExpr> >(readsSet.begin(), readsSet.end());

  std::vector<ref<Expr> > assumption;
  assumption.push_back(EqExpr::create(e, value));

  // obscure... we need to make sure that all the read indices are
  // bounds checked. if we don't do this we can end up constructing
  // invalid counterexamples because STP will happily make out of
  // bounds indices which will not get picked up. this is of utmost
  // importance if we are being backed by the CexCachingSolver.

  for (std::vector< ref<ReadExpr> >::iterator i = reads.begin(), 
         ie = reads.end(); i != ie; ++i) {
    ReadExpr *re = i->get();
    assumption.push_back(UltExpr::create(re->index, 
                                         ConstantExpr::alloc(re->updates.root->size, 
                                                             Context::get().getPointerWidth())));
  }

  ConstraintManager assume(assumption);
  for (std::vector< ref<ReadExpr> >::iterator i = reads.begin(), 
         ie = reads.end(); i != ie; ++i) {
    ref<ReadExpr> var = *i;
    ref<ConstantExpr> possible;
    bool success = S->getValue(Query(assume, var), possible);
    assert(success && "FIXME: Unhandled solver failure");    
    std::map<ref<ReadExpr>, ref<ConstantExpr> >::iterator it = found.find(var);
    bool res;
    success = S->mustBeTrue(Query(assume, EqExpr::create(var, possible)), res);
    assert(success && "FIXME: Unhandled solver failure");    
    if (res) {
      if (it != found.end()) {
        assert(possible == it->second && "Invalid ImpliedValue!");
        found.erase(it);
      }
    } else {
      if (it!=found.end()) {
        ref<Expr> binding = it->second;
        llvm::errs() << "checkForImpliedValues: " << e  << " = " << value << "\n"
                  << "\t\t implies " << var << " == " << binding
                  << " (error)\n";
        assert(0);
      }
    }
  }

  assert(found.empty());
}
