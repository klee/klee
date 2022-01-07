//===-- BranchTypes.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_BRANCHTYPES_H
#define KLEE_BRANCHTYPES_H

#include <cstdint>

/// \cond DO_NOT_DOCUMENT
#define BRANCH_TYPES                                                           \
  BTYPEDEFAULT(NONE, 0U)                                                       \
  BTYPE(Conditional, 1U)                                                       \
  BTYPE(Indirect, 2U)                                                          \
  BTYPE(Switch, 3U)                                                            \
  BTYPE(Call, 4U)                                                              \
  BTYPE(MemOp, 5U)                                                             \
  BTYPE(ResolvePointer, 6U)                                                    \
  BTYPE(Alloc, 7U)                                                             \
  BTYPE(Realloc, 8U)                                                           \
  BTYPE(Free, 9U)                                                              \
  BTYPE(GetVal, 10U)                                                           \
  BMARK(END, 10U)
/// \endcond

/** @enum BranchType
 *  @brief Reason an ExecutionState forked
 *
 *  | Value                           | Description                                                                                        |
 *  |---------------------------------|----------------------------------------------------------------------------------------------------|
 *  | `BranchType::NONE`              | default value (no branch)                                                                          |
 *  | `BranchType::Conditional`       | branch caused by `br` instruction with symbolic condition                                          |
 *  | `BranchType::Indirect`          | branch caused by `indirectbr` instruction with symbolic address                                    |
 *  | `BranchType::Switch`            | branch caused by `switch` instruction with symbolic value                                          |
 *  | `BranchType::Call`              | branch caused by `call` with symbolic function pointer                                             |
 *  | `BranchType::MemOp`             | branch caused by memory operation with symbolic address (e.g. multiple resolutions, out-of-bounds) |
 *  | `BranchType::ResolvePointer`    | branch caused by symbolic pointer                                                                  |
 *  | `BranchType::Alloc`             | branch caused by symbolic `alloc`ation size                                                        |
 *  | `BranchType::Realloc`           | branch caused by symbolic `realloc`ation size                                                      |
 *  | `BranchType::Free`              | branch caused by `free`ing symbolic pointer                                                        |
 *  | `BranchType::GetVal`            | branch caused by user-invoked concretization while seeding                                         |
 */
enum class BranchType : std::uint8_t {
  /// \cond DO_NOT_DOCUMENT
  #define BTYPEDEFAULT(N,I) N = (I),
  #define BTYPE(N,I) N = (I),
  #define BMARK(N,I) N = (I),
  BRANCH_TYPES
  /// \endcond
};


#undef BTYPEDEFAULT
#undef BTYPE
#undef BMARK
#define BTYPEDEFAULT(N,I)
#define BTYPE(N,I)
#define BMARK(N,I)

#endif
