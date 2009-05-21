//===-- FloatEvaluation.h ---------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// FIXME: Ditch this and use APFloat.

#ifndef KLEE_UTIL_FLOATS_H
#define KLEE_UTIL_FLOATS_H

#include "klee/util/Bits.h"     //bits64::truncateToNBits
#include "IntEvaluation.h" //ints::sext

#include "llvm/Support/MathExtras.h"

namespace klee {
namespace floats {

// ********************************** //
// *** Pack uint64_t into FP types ** //
// ********************************** //

// interpret the 64 bits as a double instead of a uint64_t
inline double UInt64AsDouble( uint64_t bits ) {
  double ret;
  assert( sizeof(bits) == sizeof(ret) );
  memcpy( &ret, &bits, sizeof bits );
  return ret;
}

// interpret the first 32 bits as a float instead of a uint64_t
inline float UInt64AsFloat( uint64_t bits ) {
  uint32_t tmp = bits; // ensure that we read correct bytes
  float ret;
  assert( sizeof(tmp) == sizeof(ret) );
  memcpy( &ret, &tmp, sizeof tmp );
  return ret;
}


// ********************************** //
// *** Pack FP types into uint64_t ** //
// ********************************** //

// interpret the 64 bits as a uint64_t instead of a double
inline uint64_t DoubleAsUInt64( double d ) {
  uint64_t ret;
  assert( sizeof(d) == sizeof(ret) );
  memcpy( &ret, &d, sizeof d );
  return ret;
}

// interpret the 32 bits as a uint64_t instead of as a float (add 32 0s)
inline uint64_t FloatAsUInt64( float f ) {
  uint32_t tmp;
  assert( sizeof(tmp) == sizeof(f) );
  memcpy( &tmp, &f, sizeof f );
  return (uint64_t)tmp;
}


// ********************************** //
// ************ Constants *********** //
// ********************************** //

const unsigned FLT_BITS = 32;
const unsigned DBL_BITS = 64;



// ********************************** //
// ***** LLVM Binary Operations ***** //
// ********************************** //

// add of l and r
inline uint64_t add(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return bits64::truncateToNBits(FloatAsUInt64(UInt64AsFloat(l)  + UInt64AsFloat(r)),  FLT_BITS);
  case DBL_BITS: return bits64::truncateToNBits(DoubleAsUInt64(UInt64AsDouble(l) + UInt64AsDouble(r)), DBL_BITS);
  default: assert(0 && "invalid floating point width");
  }
}

// difference of l and r
inline uint64_t sub(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return bits64::truncateToNBits(FloatAsUInt64(UInt64AsFloat(l)  - UInt64AsFloat(r)),  FLT_BITS);
  case DBL_BITS: return bits64::truncateToNBits(DoubleAsUInt64(UInt64AsDouble(l) - UInt64AsDouble(r)), DBL_BITS);
  default: assert(0 && "invalid floating point width");
  }
}

// product of l and r
inline uint64_t mul(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return bits64::truncateToNBits(FloatAsUInt64(UInt64AsFloat(l)  * UInt64AsFloat(r)),  FLT_BITS);
  case DBL_BITS: return bits64::truncateToNBits(DoubleAsUInt64(UInt64AsDouble(l) * UInt64AsDouble(r)), DBL_BITS);
  default: assert(0 && "invalid floating point width");
  }
}

// signed divide of l by r
inline uint64_t div(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return bits64::truncateToNBits(FloatAsUInt64(UInt64AsFloat(l)  / UInt64AsFloat(r)),  FLT_BITS);
  case DBL_BITS: return bits64::truncateToNBits(DoubleAsUInt64(UInt64AsDouble(l) / UInt64AsDouble(r)), DBL_BITS);
  default: assert(0 && "invalid floating point width");
  }
}

// signed modulo of l by r
inline uint64_t mod(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return bits64::truncateToNBits(FloatAsUInt64( fmod(UInt64AsFloat(l),  UInt64AsFloat(r)) ), FLT_BITS);
  case DBL_BITS: return bits64::truncateToNBits(DoubleAsUInt64( fmod(UInt64AsDouble(l), UInt64AsDouble(r)) ), DBL_BITS);
  default: assert(0 && "invalid floating point width");
  }
}


// ********************************** //
// *** LLVM Comparison Operations *** //
// ********************************** //

// determine if l represents NaN
inline bool isNaN(uint64_t l, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return llvm::IsNAN( UInt64AsFloat(l) );
  case DBL_BITS: return llvm::IsNAN( UInt64AsDouble(l) );
  default: assert(0 && "invalid floating point width");
  }
}

inline uint64_t eq(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return UInt64AsFloat(l)  == UInt64AsFloat(r);
  case DBL_BITS: return UInt64AsDouble(l) == UInt64AsDouble(r);
  default: assert(0 && "invalid floating point width");
  }
}

inline uint64_t ne(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return UInt64AsFloat(l)  != UInt64AsFloat(r);
  case DBL_BITS: return UInt64AsDouble(l) != UInt64AsDouble(r);
  default: assert(0 && "invalid floating point width");
  }
}

inline uint64_t lt(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return UInt64AsFloat(l)  < UInt64AsFloat(r);
  case DBL_BITS: return UInt64AsDouble(l) < UInt64AsDouble(r);
  default: assert(0 && "invalid floating point width");
  }
}

inline uint64_t le(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return UInt64AsFloat(l)  <= UInt64AsFloat(r);
  case DBL_BITS: return UInt64AsDouble(l) <= UInt64AsDouble(r);
  default: assert(0 && "invalid floating point width");
  }
}

inline uint64_t gt(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return UInt64AsFloat(l)  > UInt64AsFloat(r);
  case DBL_BITS: return UInt64AsDouble(l) > UInt64AsDouble(r);
  default: assert(0 && "invalid floating point width");
  }
}

inline uint64_t ge(uint64_t l, uint64_t r, unsigned inWidth) {
  switch( inWidth ) {
  case FLT_BITS: return UInt64AsFloat(l)  >= UInt64AsFloat(r);
  case DBL_BITS: return UInt64AsDouble(l) >= UInt64AsDouble(r);
  default: assert(0 && "invalid floating point width");
  }
}


// ********************************** //
// *** LLVM Conversion Operations *** //
// ********************************** //

// truncation of l (which must be a double) to float (casts a double to a float)
inline uint64_t trunc(uint64_t l, unsigned outWidth, unsigned inWidth) {
  // FIXME: Investigate this, should this not happen? Was a quick
  // patch for busybox.
  if (inWidth==64 && outWidth==64) {
    return l;
  } else {
    assert(inWidth==64 && "can only truncate from a 64-bit double");
    assert(outWidth==32 && "can only truncate to a 32-bit float");
    float trunc = (float)UInt64AsDouble(l);
    return bits64::truncateToNBits(FloatAsUInt64(trunc), outWidth);
  }
}

// extension of l (which must be a float) to double (casts a float to a double)
inline uint64_t ext(uint64_t l, unsigned outWidth, unsigned inWidth) {
  // FIXME: Investigate this, should this not happen? Was a quick
  // patch for busybox.
  if (inWidth==64 && outWidth==64) {
    return l;
  } else {
    assert(inWidth==32 && "can only extend from a 32-bit float");
    assert(outWidth==64 && "can only extend to a 64-bit double");
    double ext = (double)UInt64AsFloat(l);
    return bits64::truncateToNBits(DoubleAsUInt64(ext), outWidth);
  }
}

// conversion of l to an unsigned integer, rounding towards zero
inline uint64_t toUnsignedInt( uint64_t l, unsigned outWidth, unsigned inWidth ) {
  switch( inWidth ) {
  case FLT_BITS: return bits64::truncateToNBits((uint64_t)UInt64AsFloat(l),  outWidth );
  case DBL_BITS: return bits64::truncateToNBits((uint64_t)UInt64AsDouble(l), outWidth );
  default: assert(0 && "invalid floating point width");
  }
}

// conversion of l to a signed integer, rounding towards zero
inline uint64_t toSignedInt( uint64_t l, unsigned outWidth, unsigned inWidth ) {
  switch( inWidth ) {
  case FLT_BITS: return bits64::truncateToNBits((int64_t)UInt64AsFloat(l),  outWidth);
  case DBL_BITS: return bits64::truncateToNBits((int64_t)UInt64AsDouble(l), outWidth);
  default: assert(0 && "invalid floating point width");
  }
}

// conversion of l, interpreted as an unsigned int, to a floating point number
inline uint64_t UnsignedIntToFP( uint64_t l, unsigned outWidth ) {
  switch( outWidth ) {
  case FLT_BITS: return bits64::truncateToNBits(FloatAsUInt64((float)l),  outWidth);
  case DBL_BITS: return bits64::truncateToNBits(DoubleAsUInt64((double)l), outWidth);
  default: assert(0 && "invalid floating point width");
  }
}

// conversion of l, interpreted as a signed int, to a floating point number
inline uint64_t SignedIntToFP( uint64_t l, unsigned outWidth, unsigned inWidth ) {
  switch( outWidth ) {
  case FLT_BITS: return bits64::truncateToNBits(FloatAsUInt64((float)(int64_t)ints::sext(l, 64, inWidth)), outWidth);
  case DBL_BITS: return bits64::truncateToNBits(DoubleAsUInt64((double)(int64_t)ints::sext(l,64, inWidth)), outWidth);
  default: assert(0 && "invalid floating point width");
  }
}

} // end namespace ints
} // end namespace klee

#endif //KLEE_UTIL_FLOATS_H
