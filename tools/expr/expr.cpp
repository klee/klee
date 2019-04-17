//===-- expr.c --------------------------------------------------*- C -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the klee expression library, in particular the C
// bindings for use in non C++ applications.
//
//===----------------------------------------------------------------------===//

#include "klee-c/expr.h"

#include "klee/Expr.h"
#include "klee/ExprBuilder.h"
#include "klee/util/ArrayCache.h"

#include "llvm/Support/CBindingWrapping.h"

using namespace klee;

namespace {
// Encapsulate a builder and the associated array cache to tie lifetime of
// arrays to that of builder.
struct LibExprBuilder {
  explicit LibExprBuilder(ExprBuilder *TheBuilder) : Builder(TheBuilder) {}
  std::unique_ptr<ExprBuilder> Builder;
  ArrayCache AC;
};

} // namespace

DEFINE_SIMPLE_CONVERSION_FUNCTIONS(ref<Expr>, klee_expr_t)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(Array, klee_array_t)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(UpdateList, klee_update_list_t)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(LibExprBuilder, klee_expr_builder_t)

// Conceptually passing ref<Expr> around the ABI boundary is a bit trickier than
// it looks. Effectively, we cannot pass it out by value, so we have to copy
// constructed into a new'd ref<Expr>. It might seem like the reference count
// will be one too high, but this is not the case, as the original ref<Expr>
// returned by the C++ API will be destroyed via stack-unwinding before crossing
// the boundary. the underlying object will live on as it is captured by the
// newly allocated ref<Expr>.
// TODO: If this pattern winds up being useful (probably for passing ref<T>
// around) consider merging this generically into CBindingWrapping.h
static klee_expr_t allocating_wrap(const ref<Expr> &RefExpr) {
  ref<Expr> *TheCopiedRef = new ref<Expr>(RefExpr);
  return wrap(TheCopiedRef);
}

klee_expr_builder_t klee_expr_builder_create(void) {
  ExprBuilder *DefaultBuilder = createDefaultExprBuilder();
  LibExprBuilder *TheBuilder = new LibExprBuilder(DefaultBuilder);
  return wrap(TheBuilder);
}

void klee_expr_builder_dispose(klee_expr_builder_t builder) {
  LibExprBuilder *TheBuilder = unwrap(builder);
  delete TheBuilder;
}

int klee_expr_compare(klee_expr_t lhs, klee_expr_t rhs) {
  ref<Expr> *Lhs = unwrap(lhs);
  ref<Expr> *Rhs = unwrap(rhs);
  return Lhs->compare(*Rhs);
}

// Clients need to make sure they call this to ensure Exprs aren't held on
// forever
void klee_expr_dispose(klee_expr_t expr) {
  ref<Expr> *TheRefExpr = unwrap(expr);
  delete TheRefExpr;
}

extern klee_array_t klee_array_create(const klee_expr_builder_t builder,
                                      const char *name, uint64_t size,
                                      const uint64_t *constants, bool is_signed,
                                      klee_expr_width_t domain,
                                      klee_expr_width_t range) {
  LibExprBuilder *TheBuilder = unwrap(builder);
  std::string TheName(name);
  ArrayCache &AC = TheBuilder->AC;
  const Array *TheArray = nullptr;

  if (!constants) {
    TheArray = AC.CreateArray(TheName, size, nullptr, nullptr, domain, range);
  } else {
    std::vector<ref<ConstantExpr>> Constants;
    Constants.reserve(size);
    for (size_t I = 0; I < size; ++I) {
      llvm::APInt TheValue(range, constants[I], is_signed);
      ref<ConstantExpr> TheConstant = ConstantExpr::alloc(TheValue);
      Constants.push_back(std::move(TheConstant));
      TheArray = AC.CreateArray(TheName, size, &Constants.front(),
                                &Constants.back(), domain, range);
    }
  }

  return wrap(TheArray);
}

klee_update_list_t klee_update_list_create(const klee_array_t array) {
  Array *TheArray = unwrap(array);
  UpdateList *TheUpdateList = new UpdateList(TheArray, nullptr);
  return wrap(TheUpdateList);
}

void klee_update_list_dispose(klee_update_list_t list) {
  UpdateList *TheUpdateList = unwrap(list);
  delete TheUpdateList;
}

klee_expr_t klee_build_constant_expr(klee_expr_builder_t builder, uint64_t val,
                                     klee_expr_width_t width, bool is_signed) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  llvm::APInt TheValue(width, val, is_signed);
  ref<Expr> ConstantExpr = LibBuilder->Builder->Constant(TheValue);
  return allocating_wrap(ConstantExpr);
}

klee_expr_t klee_build_read_expr(const klee_expr_builder_t builder,
                                 const klee_update_list_t updates,
                                 const klee_expr_t index) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  UpdateList *Updates = unwrap(updates);
  ref<Expr> *Index = unwrap(index);
  ref<Expr> ReadExpr = LibBuilder->Builder->Read(*Updates, *Index);
  return allocating_wrap(ReadExpr);
}

klee_expr_t klee_build_select_expr(const klee_expr_builder_t builder,
                                   const klee_expr_t cond,
                                   const klee_expr_t lhs,
                                   const klee_expr_t rhs) {

  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Cond = unwrap(cond);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> SelectExpr = LibBuilder->Builder->Select(*Cond, *Lhs, *Rhs);
  return allocating_wrap(SelectExpr);
}

klee_expr_t klee_build_concat_expr(const klee_expr_builder_t builder,
                                   const klee_expr_t lhs,
                                   const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> ConcatExpr = LibBuilder->Builder->Concat(*Lhs, *Rhs);
  return allocating_wrap(ConcatExpr);
}

klee_expr_t klee_build_extract_expr(const klee_expr_builder_t builder,
                                    const klee_expr_t lhs, unsigned offset,
                                    klee_expr_width_t width) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> ExtractExpr = LibBuilder->Builder->Extract(*Lhs, offset, width);
  return allocating_wrap(ExtractExpr);
}

klee_expr_t klee_build_zext_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs,
                                 klee_expr_width_t width) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> ZExtExpr = LibBuilder->Builder->ZExt(*Lhs, width);
  return allocating_wrap(ZExtExpr);
}

klee_expr_t klee_build_sext_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, klee_expr_width_t width) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> SExtExpr = LibBuilder->Builder->SExt(*Lhs, width);
  return allocating_wrap(SExtExpr);
}

klee_expr_t klee_build_add_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> AddExpr = LibBuilder->Builder->Add(*Lhs, *Rhs);
  return allocating_wrap(AddExpr);
}

klee_expr_t klee_build_sub_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> SubExpr = LibBuilder->Builder->Sub(*Lhs, *Rhs);
  return allocating_wrap(SubExpr);
}

klee_expr_t klee_build_mul_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> MulExpr = LibBuilder->Builder->Mul(*Lhs, *Rhs);
  return allocating_wrap(MulExpr);
}

klee_expr_t klee_build_udiv_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> UDivExpr = LibBuilder->Builder->UDiv(*Lhs, *Rhs);
  return allocating_wrap(UDivExpr);
}

klee_expr_t klee_build_sdiv_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> SDivExpr = LibBuilder->Builder->SDiv(*Lhs, *Rhs);
  return allocating_wrap(SDivExpr);
}

klee_expr_t klee_build_urem_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> URemExpr = LibBuilder->Builder->URem(*Lhs, *Rhs);
  return allocating_wrap(URemExpr);
}

klee_expr_t klee_build_srem_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> SRemExpr = LibBuilder->Builder->SRem(*Lhs, *Rhs);
  return allocating_wrap(SRemExpr);
}

klee_expr_t klee_build_not_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> NotExpr = LibBuilder->Builder->Not(*Lhs);
  return allocating_wrap(NotExpr);
}

klee_expr_t klee_build_and_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> AndExpr = LibBuilder->Builder->And(*Lhs, *Rhs);
  return allocating_wrap(AndExpr);
}

klee_expr_t klee_build_or_expr(const klee_expr_builder_t builder,
                               const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> OrExpr = LibBuilder->Builder->Or(*Lhs, *Rhs);
  return allocating_wrap(OrExpr);
}

klee_expr_t klee_build_xor_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> XorExpr = LibBuilder->Builder->Xor(*Lhs, *Rhs);
  return allocating_wrap(XorExpr);
}

klee_expr_t klee_build_shl_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> ShlExpr = LibBuilder->Builder->Shl(*Lhs, *Rhs);
  return allocating_wrap(ShlExpr);
}

klee_expr_t klee_build_lshr_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> LShrExpr = LibBuilder->Builder->LShr(*Lhs, *Rhs);
  return allocating_wrap(LShrExpr);
}

klee_expr_t klee_build_ashr_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> AShrExpr = LibBuilder->Builder->AShr(*Lhs, *Rhs);
  return allocating_wrap(AShrExpr);
}

klee_expr_t klee_build_eq_expr(const klee_expr_builder_t builder,
                               const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> EqExpr = LibBuilder->Builder->Eq(*Lhs, *Rhs);
  return allocating_wrap(EqExpr);
}

klee_expr_t klee_build_ne_expr(const klee_expr_builder_t builder,
                               const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> NeExpr = LibBuilder->Builder->Ne(*Lhs, *Rhs);
  return allocating_wrap(NeExpr);
}

klee_expr_t klee_build_ult_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> UltExpr = LibBuilder->Builder->Ult(*Lhs, *Rhs);
  return allocating_wrap(UltExpr);
}

klee_expr_t klee_build_ule_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> UleExpr = LibBuilder->Builder->Ule(*Lhs, *Rhs);
  return allocating_wrap(UleExpr);
}

klee_expr_t klee_build_ugt_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> UgtExpr = LibBuilder->Builder->Ugt(*Lhs, *Rhs);
  return allocating_wrap(UgtExpr);
}

klee_expr_t klee_build_uge_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> UgeExpr = LibBuilder->Builder->Uge(*Lhs, *Rhs);
  return allocating_wrap(UgeExpr);
}

klee_expr_t klee_build_slt_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> SltExpr = LibBuilder->Builder->Slt(*Lhs, *Rhs);
  return allocating_wrap(SltExpr);
}

klee_expr_t klee_build_sle_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> SleExpr = LibBuilder->Builder->Sle(*Lhs, *Rhs);
  return allocating_wrap(SleExpr);
}

klee_expr_t klee_build_sgt_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> SgtExpr = LibBuilder->Builder->Sgt(*Lhs, *Rhs);
  return allocating_wrap(SgtExpr);
}

klee_expr_t klee_build_sge_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  ref<Expr> *Lhs= unwrap(lhs);
  ref<Expr> *Rhs= unwrap(rhs);
  ref<Expr> SgeExpr = LibBuilder->Builder->Sge(*Lhs, *Rhs);
  return allocating_wrap(SgeExpr);
}
