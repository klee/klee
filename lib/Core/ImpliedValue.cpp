//===-- ImpliedValue.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ImpliedValue.h"

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/Solver.h"
// FIXME: Use APInt.
#include "klee/Internal/Support/IntEvaluation.h"

#include "klee/util/ExprUtil.h"

#include <iostream>
#include <map>
#include <set>

using namespace klee;

// XXX we really want to do some sort of canonicalization of exprs
// globally so that cases below become simpler
static void _getImpliedValue(ref<Expr> e,
                             uint64_t value,
                             ImpliedValueList &results) {
  switch (e->getKind()) {
  case Expr::Constant: {
    assert(value == e->getConstantValue() && "error in implied value calculation");
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
    results.push_back(std::make_pair(re, 
                                     ConstantExpr::create(value, e->getWidth())));
    break;
  }
    
  case Expr::Select: {
    // not much to do, could improve with range analysis
    SelectExpr *se = cast<SelectExpr>(e);
    
    if (ConstantExpr *TrueCE = dyn_cast<ConstantExpr>(se->trueExpr)) {
      if (ConstantExpr *FalseCE = dyn_cast<ConstantExpr>(se->falseExpr)) {
        if (TrueCE->getConstantValue() != FalseCE->getConstantValue()) {
          if (value == TrueCE->getConstantValue()) {
            _getImpliedValue(se->cond, 1, results);
          } else {
            assert(value == FalseCE->getConstantValue() &&
                   "err in implied value calculation");
            _getImpliedValue(se->cond, 0, results);
          }
        }
      }
    }
    break;
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    _getImpliedValue(ce->getKid(0), (value >> ce->getKid(1)->getWidth()) & ((1 << ce->getKid(0)->getWidth()) - 1), results);
    _getImpliedValue(ce->getKid(1), value & ((1 << ce->getKid(1)->getWidth()) - 1), results);
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
    _getImpliedValue(ce->src, 
                     bits64::truncateToNBits(value, 
					     ce->src->getWidth()),
                     results);
    break;
  }

    // Arithmetic

  case Expr::Add: { // constants on left
    BinaryExpr *be = cast<BinaryExpr>(e);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left)) {
      uint64_t nvalue = ints::sub(value,
                                  CE->getConstantValue(),
                                  CE->getWidth());
      _getImpliedValue(be->right, nvalue, results);
    }
    break;
  }
  case Expr::Sub: { // constants on left
    BinaryExpr *be = cast<BinaryExpr>(e);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left)) {
      uint64_t nvalue = ints::sub(CE->getConstantValue(),
                                  value,
                                  CE->getWidth());
      _getImpliedValue(be->right, nvalue, results);
    }
    break;
  }
  case Expr::Mul: {
    // XXX can do stuff here, but need valid mask and other things
    // because of bits that might be lost
    break;
  }

  case Expr::UDiv:
  case Expr::SDiv:
  case Expr::URem:
  case Expr::SRem:
    // no, no, no
    break;

    // Binary

  case Expr::And: {
    BinaryExpr *be = cast<BinaryExpr>(e);
    if (be->getWidth() == Expr::Bool) {
      if (value) {
        _getImpliedValue(be->left, value, results);
        _getImpliedValue(be->right, value, results);
      }
    } else {
      // XXX, we can basically propogate a mask here
      // where we know "some bits". may or may not be
      // useful.
    }
    break;
  }
  case Expr::Or: {
    BinaryExpr *be = cast<BinaryExpr>(e);
    if (!value) {
      _getImpliedValue(be->left, 0, results);
      _getImpliedValue(be->right, 0, results);
    } else {
      // XXX, can do more?
    }
    break;
  }
  case Expr::Xor: { // constants on left
    BinaryExpr *be = cast<BinaryExpr>(e);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left)) {
      _getImpliedValue(be->right, value ^ CE->getConstantValue(), results);
    }
    break;
  }

    // Comparison
  case Expr::Ne: 
    value = !value;
    /* fallthru */
  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    if (value) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ee->left))
        _getImpliedValue(ee->right, CE->getConstantValue(), results);
    } else {
      // look for limited value range, woohoo
      //
      // in general anytime one side was restricted to two values we
      // can apply this trick. the only obvious case where this
      // occurs, aside from booleans, is as the result of a select
      // expression where the true and false branches are single
      // valued and distinct.
      
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ee->left)) {
        if (CE->getWidth() == Expr::Bool) {
          _getImpliedValue(ee->right, !CE->getConstantValue(), results);
        }
      }
    }
    break;
  }
    
  default:
    break;
  }
}
    
void ImpliedValue::getImpliedValues(ref<Expr> e, 
                                    ref<ConstantExpr> value, 
                                    ImpliedValueList &results) {
  _getImpliedValue(e, value->getConstantValue(), results);
}

void ImpliedValue::checkForImpliedValues(Solver *S, ref<Expr> e, 
                                         ref<ConstantExpr> value) {
  std::vector<ref<ReadExpr> > reads;
  std::map<ref<ReadExpr>, ref<Expr> > found;
  ImpliedValueList results;

  getImpliedValues(e, value, results);

  for (ImpliedValueList::iterator i = results.begin(), ie = results.end();
       i != ie; ++i) {
    std::map<ref<ReadExpr>, ref<Expr> >::iterator it = found.find(i->first);
    if (it != found.end()) {
      assert(it->second->getConstantValue() == i->second->getConstantValue() &&
             "I don't think so Scott");
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
                                                             kMachinePointerType)));
  }

  ConstraintManager assume(assumption);
  for (std::vector< ref<ReadExpr> >::iterator i = reads.begin(), 
         ie = reads.end(); i != ie; ++i) {
    ref<ReadExpr> var = *i;
    ref<Expr> possible;
    bool success = S->getValue(Query(assume, var), possible);
    assert(success && "FIXME: Unhandled solver failure");    
    std::map<ref<ReadExpr>, ref<Expr> >::iterator it = found.find(var);
    bool res;
    success = S->mustBeTrue(Query(assume, EqExpr::create(var, possible)), res);
    assert(success && "FIXME: Unhandled solver failure");    
    if (res) {
      if (it != found.end()) {
        assert(possible->getConstantValue() == it->second->getConstantValue());
        found.erase(it);
      }
    } else {
      if (it!=found.end()) {
        ref<Expr> binding = it->second;
        llvm::cerr << "checkForImpliedValues: " << e  << " = " << value << "\n"
                   << "\t\t implies " << var << " == " << binding
                   << " (error)\n";
        assert(0);
      }
    }
  }

  assert(found.empty());
}
