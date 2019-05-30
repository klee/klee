//===-- Bits.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_BITS_H
#define KLEE_BITS_H

#include "klee/Config/Version.h"
#include "llvm/Support/DataTypes.h"
#include <assert.h>

namespace klee {
  namespace bits32 {
    // @pre(0 <= N <= 32)
    // @post(retval = max([truncateToNBits(i,N) for i in naturals()]))
    inline unsigned maxValueOfNBits(unsigned N) {
      assert(N <= 32);
      if (N==0)
        return 0;
      return (UINT32_C(-1)) >> (32 - N);
    }

    // @pre(0 < N <= 32)
    inline unsigned truncateToNBits(unsigned x, unsigned N) {
      assert(N > 0 && N <= 32);
      return x&((UINT32_C(-1)) >> (32 - N));
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
      assert(withoutRightmostBit(x) == 0);
      unsigned res = 0;
      if (x&0xFFFF0000) res += 16;
      if (x&0xFF00FF00) res += 8;
      if (x&0xF0F0F0F0) res += 4;
      if (x&0xCCCCCCCC) res += 2;
      if (x&0xAAAAAAAA) res += 1;
      assert(res < 32);
      assert((UINT32_C(1) << res) == x);
      return res;
    } 

    inline unsigned indexOfRightmostBit(unsigned x) {
      return indexOfSingleBit(isolateRightmostBit(x));
    }
  }

  namespace bits64 {
    // @pre(0 <= N <= 64)
    // @post(retval = max([truncateToNBits(i,N) for i in naturals()]))
    inline uint64_t maxValueOfNBits(unsigned N) {
      assert(N <= 64);
      if (N==0)
        return 0;
      return ((UINT64_C(-1)) >> (64 - N));
    }
    
    // @pre(0 < N <= 64)
    inline uint64_t truncateToNBits(uint64_t x, unsigned N) {
      assert(N > 0 && N <= 64);
      return x&((UINT64_C(-1)) >> (64 - N));
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
      assert((x & (x - 1)) == 0);
      unsigned res = bits32::indexOfSingleBit((unsigned) (x | (x>>32)));
      if (x & (UINT64_C(0xFFFFFFFF) << 32))
        res += 32;
      assert(res < 64);
      assert((UINT64_C(1) << res) == x);
      return res;
    } 

    inline uint64_t indexOfRightmostBit(uint64_t x) {
      return indexOfSingleBit(isolateRightmostBit(x));
    }
  }
} // End klee namespace

#endif /* KLEE_BITS_H */
