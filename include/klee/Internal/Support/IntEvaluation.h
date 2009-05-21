//===-- IntEvaluation.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_UTIL_INTEVALUATION_H
#define KLEE_UTIL_INTEVALUATION_H

#include "klee/util/Bits.h"

#define MAX_BITS (sizeof(uint64_t) * 8)

// ASSUMPTION: invalid bits in each uint64_t are 0. the trade-off here is
// between making trunc/zext/sext fast and making operations that depend
// on the invalid bits being 0 fast. 

namespace klee {
namespace ints {

// add of l and r
inline uint64_t add(uint64_t l, uint64_t r, unsigned inWidth) {
  return bits64::truncateToNBits(l + r, inWidth);
}

// difference of l and r
inline uint64_t sub(uint64_t l, uint64_t r, unsigned inWidth) {
  return bits64::truncateToNBits(l - r, inWidth);
}

// product of l and r
inline uint64_t mul(uint64_t l, uint64_t r, unsigned inWidth) {
  return bits64::truncateToNBits(l * r, inWidth);
}

// truncation of l to outWidth bits
inline uint64_t trunc(uint64_t l, unsigned outWidth, unsigned inWidth) {
  return bits64::truncateToNBits(l, outWidth);
}

// zero-extension of l from inWidth to outWidth bits
inline uint64_t zext(uint64_t l, unsigned outWidth, unsigned inWidth) {
  return l;
}

// sign-extension of l from inWidth to outWidth bits
inline uint64_t sext(uint64_t l, unsigned outWidth, unsigned inWidth) {
  uint32_t numInvalidBits = MAX_BITS - inWidth;
  return bits64::truncateToNBits(((int64_t)(l << numInvalidBits)) >> numInvalidBits, outWidth);
}

// unsigned divide of l by r
inline uint64_t udiv(uint64_t l, uint64_t r, unsigned inWidth) {
  return bits64::truncateToNBits(l / r, inWidth);
}

// unsigned mod of l by r
inline uint64_t urem(uint64_t l, uint64_t r, unsigned inWidth) {
  return bits64::truncateToNBits(l % r, inWidth);
}

// signed divide of l by r
inline uint64_t sdiv(uint64_t l, uint64_t r, unsigned inWidth) {
  // sign extend operands so that signed operation on 64-bits works correctly
  int64_t sl = sext(l, MAX_BITS, inWidth);
  int64_t sr = sext(r, MAX_BITS, inWidth);
  return bits64::truncateToNBits(sl / sr, inWidth);
}

// signed mod of l by r
inline uint64_t srem(uint64_t l, uint64_t r, unsigned inWidth) {
  // sign extend operands so that signed operation on 64-bits works correctly
  int64_t sl = sext(l, MAX_BITS, inWidth);
  int64_t sr = sext(r, MAX_BITS, inWidth);
  return bits64::truncateToNBits(sl % sr, inWidth);
}

// arithmetic shift right of l by r bits
inline uint64_t ashr(uint64_t l, uint64_t shift, unsigned inWidth) {
  int64_t sl = sext(l, MAX_BITS, inWidth);
  return bits64::truncateToNBits(sl >> shift, inWidth);
}

// logical shift right of l by r bits
inline uint64_t lshr(uint64_t l, uint64_t shift, unsigned inWidth) {
  return l >> shift;
}

// shift left of l by r bits
inline uint64_t shl(uint64_t l, uint64_t shift, unsigned inWidth) {
  return bits64::truncateToNBits(l << shift, inWidth);
}

// logical AND of l and r
inline uint64_t land(uint64_t l, uint64_t r, unsigned inWidth) {
  return l & r;
}

// logical OR of l and r
inline uint64_t lor(uint64_t l, uint64_t r, unsigned inWidth) {
  return l | r;
}

// logical XOR of l and r
inline uint64_t lxor(uint64_t l, uint64_t r, unsigned inWidth) {
  return l ^ r;
}

// comparison operations
inline uint64_t eq(uint64_t l, uint64_t r, unsigned inWidth) {
  return l == r;
}

inline uint64_t ne(uint64_t l, uint64_t r, unsigned inWidth) {
  return l != r;
}

inline uint64_t ult(uint64_t l, uint64_t r, unsigned inWidth) {
  return l < r;
}

inline uint64_t ule(uint64_t l, uint64_t r, unsigned inWidth) {
  return l <= r;
}

inline uint64_t ugt(uint64_t l, uint64_t r, unsigned inWidth) {
  return l > r;
}

inline uint64_t uge(uint64_t l, uint64_t r, unsigned inWidth) {
  return l >= r;
}

inline uint64_t slt(uint64_t l, uint64_t r, unsigned inWidth) {
  int64_t sl = sext(l, MAX_BITS, inWidth);
  int64_t sr = sext(r, MAX_BITS, inWidth);
  return sl < sr;
}

inline uint64_t sle(uint64_t l, uint64_t r, unsigned inWidth) {
  int64_t sl = sext(l, MAX_BITS, inWidth);
  int64_t sr = sext(r, MAX_BITS, inWidth);
  return sl <= sr;
}

inline uint64_t sgt(uint64_t l, uint64_t r, unsigned inWidth) {
  int64_t sl = sext(l, MAX_BITS, inWidth);
  int64_t sr = sext(r, MAX_BITS, inWidth);
  return sl > sr;
}

inline uint64_t sge(uint64_t l, uint64_t r, unsigned inWidth) {
  int64_t sl = sext(l, MAX_BITS, inWidth);
  int64_t sr = sext(r, MAX_BITS, inWidth);
  return sl >= sr;
}

} // end namespace ints
} // end namespace klee

#endif 
