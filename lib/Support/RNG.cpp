/* 
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.
   Modified to be a C++ class by Daniel Dunbar.

   Before using, initialize the state by using init_genrand(seed)  
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#include "klee/ADT/RNG.h"
#include "klee/Support/OptionCategories.h"

using namespace klee;

namespace {
llvm::cl::opt<unsigned> RNGInitialSeed(
    "rng-initial-seed", llvm::cl::init(5489U),
    llvm::cl::desc("seed value for random number generator (default=5489)"),
    llvm::cl::cat(klee::MiscCat));

}

RNG::RNG() {
  seed(RNGInitialSeed);
}

/* initializes mt[N] with a seed */
RNG::RNG(unsigned int s) {
  seed(s);
}

void RNG::seed(unsigned int s) {
  mt[0]= s & 0xffffffffUL;
  for (mti=1; mti<N; mti++) {
    mt[mti] = 
      (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30U)) + mti);
    /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
    /* In the previous versions, MSBs of the seed affect   */
    /* only MSBs of the array mt[].                        */
    /* 2002/01/09 modified by Makoto Matsumoto             */
    mt[mti] &= 0xffffffffUL;
    /* for >32 bit machines */
  }
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned int RNG::getInt32() {
  unsigned int y;
  static unsigned int mag01[2]={0x0UL, MATRIX_A};
  /* mag01[x] = x * MATRIX_A  for x=0,1 */

  if (mti >= N) { /* generate N words at one time */
    int kk;

    for (kk=0;kk<N-M;kk++) {
      y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
      mt[kk] = mt[kk+M] ^ (y >> 1U) ^ mag01[y & 0x1UL];
    }
    for (;kk<N-1;kk++) {
      y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
      mt[kk] = mt[kk+(M-N)] ^ (y >> 1U) ^ mag01[y & 0x1UL];
    }
    y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
    mt[N-1] = mt[M-1] ^ (y >> 1U) ^ mag01[y & 0x1UL];

    mti = 0;
  }

  y = mt[mti++];

  /* Tempering */
  y ^= (y >> 11U);
  y ^= (y << 7U) & 0x9d2c5680UL;
  y ^= (y << 15U) & 0xefc60000UL;
  y ^= (y >> 18U);

  return y;
}

/* generates a random number on [0,0x7fffffff]-interval */
int RNG::getInt31() {
  return (int)(getInt32() >> 1U);
}

/* generates a random number on [0,1]-real-interval */
double RNG::getDoubleLR() {
  return getInt32()*(1.0/4294967295.0); 
  /* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double RNG::getDoubleL() {
  return getInt32()*(1.0/4294967296.0); 
  /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double RNG::getDouble() {
  return (((double)getInt32()) + 0.5)*(1.0/4294967296.0); 
  /* divided by 2^32 */
}

float RNG::getFloatLR() {
  return getInt32()*(1.0f/4294967295.0f); 
  /* divided by 2^32-1 */ 
}
float RNG::getFloatL() {
  return getInt32()*(1.0f/4294967296.0f); 
  /* divided by 2^32 */
}
float RNG::getFloat() {
  return (getInt32() + 0.5f)*(1.0f/4294967296.0f); 
  /* divided by 2^32 */
}

bool RNG::getBool() {
  unsigned bits = getInt32();
  bits ^= bits >> 16U;
  bits ^= bits >> 8U;
  bits ^= bits >> 4U;
  bits ^= bits >> 2U;
  bits ^= bits >> 1U;
  return bits & 1U;
}
