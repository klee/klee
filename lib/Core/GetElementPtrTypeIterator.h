//===-- GetElementPtrTypeIterator.h -----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements an iterator for walking through the types indexed by
// getelementptr, insertvalue and extractvalue instructions.
//
// It is an enhanced version of llvm::gep_type_iterator which only handles
// getelementptr.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_GETELEMENTPTRTYPEITERATOR_H
#define KLEE_GETELEMENTPTRTYPEITERATOR_H

#include "llvm/IR/User.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"

#include "klee/Config/Version.h"

namespace klee {
template <typename ItTy = llvm::User::const_op_iterator>
class generic_gep_type_iterator
    : public std::iterator<std::forward_iterator_tag, llvm::Type *, ptrdiff_t> {
  typedef std::iterator<std::forward_iterator_tag, llvm::Type *, ptrdiff_t>
      super;

    ItTy OpIt;
    llvm::Type *CurTy;
    generic_gep_type_iterator() {}

    llvm::Value *asValue(llvm::Value *V) const { return V; }
    llvm::Value *asValue(unsigned U) const {
      return llvm::ConstantInt::get(CurTy->getContext(), llvm::APInt(32, U));
    }

  public:
    static generic_gep_type_iterator begin(llvm::Type *Ty, ItTy It) {
      generic_gep_type_iterator I;
      I.CurTy = Ty;
      I.OpIt = It;
      return I;
    }
    static generic_gep_type_iterator end(ItTy It) {
      generic_gep_type_iterator I;
      I.CurTy = 0;
      I.OpIt = It;
      return I;
    }

    bool operator==(const generic_gep_type_iterator& x) const {
      return OpIt == x.OpIt;
    }
    bool operator!=(const generic_gep_type_iterator& x) const {
      return !operator==(x);
    }

    llvm::Type *operator*() const { return CurTy; }

    llvm::Type *getIndexedType() const {
      llvm::CompositeType *CT = cast<llvm::CompositeType>(CurTy);
      return CT->getTypeAtIndex(getOperand());
    }

    // This is a non-standard operator->.  It allows you to call methods on the
    // current type directly.
    llvm::Type *operator->() const { return operator*(); }

    llvm::Value *getOperand() const { return asValue(*OpIt); }

    generic_gep_type_iterator& operator++() {   // Preincrement
      if (llvm::CompositeType *CT = dyn_cast<llvm::CompositeType>(CurTy)) {
        CurTy = CT->getTypeAtIndex(getOperand());
#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
      } else if (auto ptr = dyn_cast<llvm::PointerType>(CurTy)) {
        CurTy = ptr->getElementType();
#endif
      } else {
        CurTy = 0;
      }
      ++OpIt;
      return *this;
    }

    generic_gep_type_iterator operator++(int) { // Postincrement
      generic_gep_type_iterator tmp = *this; ++*this; return tmp;
    }
  };

  typedef generic_gep_type_iterator<> gep_type_iterator;
  typedef generic_gep_type_iterator<llvm::ExtractValueInst::idx_iterator> ev_type_iterator;
  typedef generic_gep_type_iterator<llvm::InsertValueInst::idx_iterator> iv_type_iterator;
  typedef generic_gep_type_iterator<llvm::SmallVector<unsigned, 4>::const_iterator> vce_type_iterator;

  inline gep_type_iterator gep_type_begin(const llvm::User *GEP) {
    return gep_type_iterator::begin(GEP->getOperand(0)->getType(),
                                    GEP->op_begin()+1);
  }
  inline gep_type_iterator gep_type_end(const llvm::User *GEP) {
    return gep_type_iterator::end(GEP->op_end());
  }
  inline gep_type_iterator gep_type_begin(const llvm::User &GEP) {
    return gep_type_iterator::begin(GEP.getOperand(0)->getType(),
                                    GEP.op_begin()+1);
  }
  inline gep_type_iterator gep_type_end(const llvm::User &GEP) {
    return gep_type_iterator::end(GEP.op_end());
  }

  inline ev_type_iterator ev_type_begin(const llvm::ExtractValueInst *EV) {
    return ev_type_iterator::begin(EV->getOperand(0)->getType(),
                                   EV->idx_begin());
  }
  inline ev_type_iterator ev_type_end(const llvm::ExtractValueInst *EV) {
    return ev_type_iterator::end(EV->idx_end());
  }

  inline iv_type_iterator iv_type_begin(const llvm::InsertValueInst *IV) {
    return iv_type_iterator::begin(IV->getType(),
                                   IV->idx_begin());
  }
  inline iv_type_iterator iv_type_end(const llvm::InsertValueInst *IV) {
    return iv_type_iterator::end(IV->idx_end());
  }

  inline vce_type_iterator vce_type_begin(const llvm::ConstantExpr *CE) {
    return vce_type_iterator::begin(CE->getOperand(0)->getType(),
                                    CE->getIndices().begin());
  }
  inline vce_type_iterator vce_type_end(const llvm::ConstantExpr *CE) {
    return vce_type_iterator::end(CE->getIndices().end());
  }

  template <typename ItTy>
  inline generic_gep_type_iterator<ItTy> gep_type_begin(llvm::Type *Op0, ItTy I,
                                                        ItTy E) {
    return generic_gep_type_iterator<ItTy>::begin(Op0, I);
  }

  template <typename ItTy>
  inline generic_gep_type_iterator<ItTy> gep_type_end(llvm::Type *Op0, ItTy I,
                                                      ItTy E) {
    return generic_gep_type_iterator<ItTy>::end(E);
  }
} // end namespace klee

#endif /* KLEE_GETELEMENTPTRTYPEITERATOR_H */
