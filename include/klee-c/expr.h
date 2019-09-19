//===-- expr.h --------------------------------------------------*- C -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_C_EXPR_H
#define KLEE_C_EXPR_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

#define KLEE_C_EXPR_API_VERSION 1

// Make sure this is kept in sync with klee::Expr::Width
typedef unsigned klee_expr_width_t;

// Opaque wrapping type for ref<Expr>
typedef struct OpaqueExpr *klee_expr_t;

// Opaque wrapping type for Array
typedef struct OpaqueArray *klee_array_t;

// Opaque wrapping type for UpdateList
typedef struct OpaqueUpdateList *klee_update_list_t;

// Opaque wrapping type for ExprBuilder
typedef struct OpaqueExprBuilder *klee_expr_builder_t;

typedef void (*registration_fn_t)(klee_expr_t);

#ifdef __cplusplus
extern "C" {
#endif
/// Builds an expression builder for use in the client
extern klee_expr_builder_t
klee_expr_builder_create(registration_fn_t registration_fn);

/// Destroys a klee_expr_builder_t
extern void klee_expr_builder_dispose(klee_expr_builder_t builder);

extern klee_expr_width_t klee_expr_get_width(klee_expr_t expr);

extern bool klee_expr_is_constant(klee_expr_t expr);

/// Compares klee_expr_t for structural equivalence
///
/// param [lhs] The expression to compare to
/// param [rhs] The expression to compare against
///
/// \return One of the following values:
///
/// * -1 iff `lhs` is `<` `rhs`
/// * 0 iff `lhs` is structurally equivalent to `rhs`
/// * 1 iff `lhs` is `>` `rhs`
///
extern int klee_expr_compare(klee_expr_t lhs, klee_expr_t rhs);

/// Destroys a klee_expr_t
extern void klee_expr_dispose(klee_expr_t expr);

/// Copies the underlying expression so that the caller can have a new reference
/// to the expression. This expression will be unregistered, it is the caller's
/// responisibility to register it manually.
///
/// param [expr] The expression to copy
extern klee_expr_t klee_expr_copy(klee_expr_t expr);

extern void klee_expr_dump(klee_expr_t expr);

/// Builds an array for use in subsequent expressions
extern klee_array_t klee_array_create(const char *name, uint64_t size,
                                      const uint64_t *constants, bool is_signed,
                                      klee_expr_width_t domain,
                                      klee_expr_width_t range);

/// Destroys a klee_arry_t
extern void klee_array_dispose(klee_array_t array);

/// Builds an update list for use in array based expression
extern klee_update_list_t klee_update_list_create(const klee_array_t array);

extern void klee_update_list_extend(klee_update_list_t updates, klee_expr_t idx,
                                    klee_expr_t value);

/// Destroys a klee_update_list
extern void klee_update_list_dispose(klee_update_list_t list);

extern klee_expr_t klee_build_constant_expr(const klee_expr_builder_t builder,
                                            uint64_t val,
                                            klee_expr_width_t width,
                                            bool is_signed);

extern klee_expr_t klee_build_read_expr(const klee_expr_builder_t builder,
                                        const klee_update_list_t updates,
                                        const klee_expr_t index);

extern klee_expr_t klee_build_select_expr(const klee_expr_builder_t builder,
                                          const klee_expr_t cond,
                                          const klee_expr_t lhs,
                                          const klee_expr_t rhs);

extern klee_expr_t klee_build_concat_expr(const klee_expr_builder_t builder,
                                          const klee_expr_t lhs,
                                          const klee_expr_t rhs);

extern klee_expr_t klee_build_extract_expr(const klee_expr_builder_t builder,
                                           const klee_expr_t lhs,
                                           unsigned offset,
                                           klee_expr_width_t width);

extern klee_expr_t klee_build_zext_expr(const klee_expr_builder_t builder,
                                        const klee_expr_t lhs,
                                        klee_expr_width_t width);

extern klee_expr_t klee_build_sext_expr(const klee_expr_builder_t builder,
                                        const klee_expr_t lhs,
                                        klee_expr_width_t width);

extern klee_expr_t klee_build_add_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_sub_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_mul_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_udiv_expr(const klee_expr_builder_t builder,
                                        const klee_expr_t lhs,
                                        const klee_expr_t rhs);

extern klee_expr_t klee_build_sdiv_expr(const klee_expr_builder_t builder,
                                        const klee_expr_t lhs,
                                        const klee_expr_t rhs);

extern klee_expr_t klee_build_urem_expr(const klee_expr_builder_t builder,
                                        const klee_expr_t lhs,
                                        const klee_expr_t rhs);

extern klee_expr_t klee_build_srem_expr(const klee_expr_builder_t builder,
                                        const klee_expr_t lhs,
                                        const klee_expr_t rhs);

extern klee_expr_t klee_build_not_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs);

extern klee_expr_t klee_build_and_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_or_expr(const klee_expr_builder_t builder,
                                      const klee_expr_t lhs,
                                      const klee_expr_t rhs);

extern klee_expr_t klee_build_xor_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_shl_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_lshr_expr(const klee_expr_builder_t builder,
                                        const klee_expr_t lhs,
                                        const klee_expr_t rhs);

extern klee_expr_t klee_build_ashr_expr(const klee_expr_builder_t builder,
                                        const klee_expr_t lhs,
                                        const klee_expr_t rhs);

extern klee_expr_t klee_build_eq_expr(const klee_expr_builder_t builder,
                                      const klee_expr_t lhs,
                                      const klee_expr_t rhs);

extern klee_expr_t klee_build_ne_expr(const klee_expr_builder_t builder,
                                      const klee_expr_t lhs,
                                      const klee_expr_t rhs);

extern klee_expr_t klee_build_ult_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_ule_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_ugt_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_uge_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_slt_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_sle_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_sgt_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

extern klee_expr_t klee_build_sge_expr(const klee_expr_builder_t builder,
                                       const klee_expr_t lhs,
                                       const klee_expr_t rhs);

#ifdef __cplusplus
}
#endif

#endif
