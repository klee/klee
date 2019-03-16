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
#include "klee/Internal/Support/CBindingWrapping.h"

#include "klee/Expr.h"
#include "klee/ExprBuilder.h"
#include "klee/util/ArrayCache.h"

using namespace klee;

static ArrayCache AC;

namespace {
// Encapsulate a builder and the associated array cache to tie lifetime of
// arrays to that of builder.
struct LibExprBuilder {
  explicit LibExprBuilder(ExprBuilder *TheBuilder) : Builder(TheBuilder) {}
  std::unique_ptr<ExprBuilder> Builder;
  ArrayCache AC;
};

} // namespace

KLEE_DEFINE_C_WRAPPERS(ref<Expr>, klee_expr_t)
KLEE_DEFINE_C_WRAPPERS(Array, klee_array_t)
KLEE_DEFINE_C_WRAPPERS(UpdateList, klee_update_list_t)
KLEE_DEFINE_C_WRAPPERS(LibExprBuilder, klee_expr_builder_t)

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
      llvm::APInt TheValue(constants[I], range, is_signed);
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

void klee_update_list_destroy(klee_update_list_t list) {
  UpdateList *TheUpdateList = unwrap(list);
  delete TheUpdateList;
}

klee_expr_t klee_expr_build_constant(klee_expr_builder_t builder, uint64_t val,
                                     klee_expr_width_t width, bool is_signed) {
  LibExprBuilder *LibBuilder = unwrap(builder);
  llvm::APInt TheValue(val, width, is_signed);
  auto Expr = LibBuilder->Builder->Constant(TheValue);
  return allocating_wrap(Expr);
}

klee_expr_t klee_build_read_expr(const klee_expr_builder_t builder,
                                 const klee_update_list_t updates,
                                 const klee_expr_t index) {
  assert(false && "NYP");
}

klee_expr_t klee_build_select_expr(const klee_expr_builder_t builder,
                                   const klee_expr_t cond,
                                   const klee_expr_t lhs,
                                   const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_concat_expr(const klee_expr_builder_t builder,
                                   const klee_expr_t lhs,
                                   const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_extract_expr(const klee_expr_builder_t builder,
                                    const klee_expr_t lhs, unsigned offset,
                                    klee_expr_width_t width) {
  assert(false && "NYP");
}

klee_expr_t klee_build_zext_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs,
                                 klee_expr_width_t width) {
  assert(false && "NYP");
}

klee_expr_t klee_build_sext_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, klee_expr_width_t W) {
  assert(false && "NYP");
}

klee_expr_t klee_build_add_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_sub_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_mul_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_udiv_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_sdiv_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_urem_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_srem_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_not_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_and_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_or_expr(const klee_expr_builder_t builder,
                               const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_xor_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_shl_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_lshr_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_ashr_expr(const klee_expr_builder_t builder,
                                 const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_eq_expr(const klee_expr_builder_t builder,
                               const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_ne_expr(const klee_expr_builder_t builder,
                               const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_ult_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_ule_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_ugt_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_uge_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_slt_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_sle_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_sgt_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}

klee_expr_t klee_build_sge_expr(const klee_expr_builder_t builder,
                                const klee_expr_t lhs, const klee_expr_t rhs) {
  assert(false && "NYP");
}
