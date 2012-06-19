//===-- Bits.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_UTIL_BITS_H
#define KLEE_UTIL_BITS_H

#include "klee/Config/Version.h"
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9) && LLVM_VERSION_CODE >= LLVM_VERSION(2, 7)
#include "llvm/System/DataTypes.h"
#else
#include "llvm/Support/DataTypes.h"
#endif

namespace klee {
  namespace bits32 {
    // @pre(0 <= N <= 32)
    // @post(retval = max([truncateToNBits(i,N) for i in naturals()]))
    inline unsigned maxValueOfNBits(unsigned N) {
      if (N==0)
        return 0;
      return ((unsigned) -1) >> (32 - N);
    }

    // @pre(0 < N <= 32)
    inline unsigned truncateToNBits(unsigned x, unsigned N) {
      return x&(((unsigned) -1) >> (32 - N));
    }

    inline unsigned withoutRightmostBit(unsigned x) {
      return x&(x-1);
    }

    inline unsigned isolateRightmostBit(unsigned x) {
      return x&-x;
    }

    inline unsigned isPowerOfTwo(unsigned x) {
      if (x==0) return 0;
      return !(x&(x-1));
    }

    // @pre(withoutRightmostBit(x) == 0)
    // @post((1 << retval) == x)
    inline unsigned indexOfSingleBit(unsigned x) {
      unsigned res = 0;
      if (x&0xFFFF0000) res += 16;
      if (x&0xFF00FF00) res += 8;
      if (x&0xF0F0F0F0) res += 4;
      if (x&0xCCCCCCCC) res += 2;
      if (x&0xAAAAAAAA) res += 1;
      return res;
    } 

    inline unsigned indexOfRightmostBit(unsigned x) {
      return indexOfSingleBit(isolateRightmostBit(x));
    }
  }

  namespace bits64 {
    // @pre(0 <= N <= 32)
    // @post(retval = max([truncateToNBits(i,N) for i in naturals()]))
    inline uint64_t maxValueOfNBits(unsigned N) {
      if (N==0)
        return 0;
      return ((uint64_t) (int64_t) -1) >> (64 - N);
    }
    
    // @pre(0 < N <= 64)
    inline uint64_t truncateToNBits(uint64_t x, unsigned N) {
      return x&(((uint64_t) (int64_t) -1) >> (64 - N));
    }

    inline uint64_t withoutRightmostBit(uint64_t x) {
      return x&(x-1);
    }

    inline uint64_t isolateRightmostBit(uint64_t x) {
      return x&-x;
    }

    inline uint64_t isPowerOfTwo(uint64_t x) {
      if (x==0) return 0;
      return !(x&(x-1));
    }

    // @pre((x&(x-1)) == 0)
    // @post((1 << retval) == x)
    inline unsigned indexOfSingleBit(uint64_t x) {
      unsigned res = bits32::indexOfSingleBit((unsigned) (x | (x>>32)));
      if (x&((uint64_t) 0xFFFFFFFF << 32))
	  res += 32;
      return res;
    } 

    inline uint64_t indexOfRightmostBit(uint64_t x) {
      return indexOfSingleBit(isolateRightmostBit(x));
    }
  }
} // End klee namespace

#endif
