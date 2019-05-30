//===-- ConstantDivision.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ConstantDivision.h"

#include "klee/util/Bits.h"

#include <algorithm>
#include <cassert>

namespace klee {

/* Macros and functions which define the basic bit-level operations 
 * needed to implement quick division operations.
 *
 * Based on Hacker's Delight (2003) by Henry S. Warren, Jr.
 */

/* 32 -- number of bits in the integer type on this architecture */

/* 2^32 -- NUM_BITS=32 requires 64 bits to represent this unsigned value */
#define TWO_TO_THE_32_U64 (1ULL << 32)

/* 2e31 -- NUM_BITS=32 requires 64 bits to represent this signed value */
#define TWO_TO_THE_31_S64 (1LL << 31)

/* ABS(x) -- positive x */
#define ABS(x) ( ((x)>0)?x:-(x) ) /* fails if x is the min value of its type */

/* XSIGN(x) -- -1 if x<0 and 0 otherwise */
#define XSIGN(x) ( (x) >> 31 )

/* LOG2_CEIL(x) -- logarithm base 2 of x, rounded up */
#define LOG2_CEIL(x) ( 32 - ldz(x - 1) )

/* ones(x) -- counts the number of bits in x with the value 1 */
static uint32_t ones(uint32_t x) {
  x -= ((x >> 1) & 0x55555555);
  x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
  x = (((x >> 4) + x) & 0x0f0f0f0f);
  x += (x >> 8);
  x += (x >> 16);

  return x & 0x0000003f;
}

/* ldz(x) -- counts the number of leading zeroes in a 32-bit word */
static uint32_t ldz(uint32_t x) {
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);

  return 32 - ones(x);
}

/* exp_base_2(n) -- 2^n computed as an integer */
static uint32_t exp_base_2(int32_t n) {
  uint32_t x = ~n & (n - 32);
  x = x >> 31;
  return x << n;
}

// A simple algorithm: Iterate over all contiguous regions of 1 bits
// in x starting with the lowest bits.
//
// For a particular range where x is 1 for bits [low,high) then:
//   1) if the range is just one bit, simple add it
//   2) if the range is more than one bit, replace with an add
//      of the high bit and a subtract of the low bit. we apply
//      one useful optimization: if we were going to add the bit
//      below the one we wish to subtract, we simply change that
//      add to a subtract instead of subtracting the low bit itself.
// Obviously we must take care when high==64.
void ComputeMultConstants64(uint64_t multiplicand, 
                            uint64_t &add, uint64_t &sub) {
  uint64_t x = multiplicand;
  add = sub = 0;

  while (x) {
    // Determine rightmost contiguous region of 1s.
    unsigned low = bits64::indexOfRightmostBit(x);
    uint64_t lowbit = 1LL << low;
    uint64_t p = x + lowbit;
    uint64_t q = bits64::isolateRightmostBit(p);
    unsigned high = q ? bits64::indexOfSingleBit(q) : 64;

    if (high==low+1) { // Just one bit...
      add |= lowbit;
    } else {
      // Rewrite as +(1<<high) - (1<<low).

      // Optimize +(1<<x) - (1<<(x+1)) to -(1<<x).
      if (low && (add & (lowbit>>1))) {
        add ^= lowbit>>1;
        sub ^= lowbit>>1;
      } else {
        sub |= lowbit;
      }

      if (high!=64)
        add |= 1LL << high;
    }

    x = p ^ q;
  }

  assert(multiplicand == add - sub);
}

void ComputeUDivConstants32(uint32_t d, uint32_t &mprime, uint32_t &sh1, 
                            uint32_t &sh2) {
  int32_t l = LOG2_CEIL( d ); /* signed so l-1 => -1 when l=0 (see sh2) */
  uint32_t mid = exp_base_2(l) - d;

  mprime = (TWO_TO_THE_32_U64 * mid / d) + 1;
  sh1    = std::min( l,   1 );
  sh2    = std::max( l-1, 0 );
}

void ComputeSDivConstants32(int32_t d, int32_t &mprime, int32_t &dsign, 
                            int32_t &shpost ) {
  uint64_t abs_d = ABS( (int64_t)d ); /* use 64-bits in case d is INT32_MIN */

  /* LOG2_CEIL works on 32-bits, so we cast abs_d.  The only possible value
   * outside the 32-bit rep. is 2^31.  This is special cased to save computer 
   * time since 64-bit routines would be overkill. */
  int32_t l = std::max( 1U, LOG2_CEIL((uint32_t)abs_d) );
  if( abs_d == TWO_TO_THE_31_S64 ) l = 31;

  uint32_t mid = exp_base_2( l - 1 );
  uint64_t m = TWO_TO_THE_32_U64 * mid / abs_d + 1ULL;

  mprime = m - TWO_TO_THE_32_U64; /* implicit cast to 32-bits signed */
  dsign  = XSIGN( d );
  shpost = l - 1;
}

}
