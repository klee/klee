//===-- ConstantDivision.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CONSTANTDIVISION_H
#define KLEE_CONSTANTDIVISION_H

#include <stdint.h>

namespace klee {

/// ComputeMultConstants64 - Compute add and sub such that add-sub==x,
/// while attempting to minimize the number of bits in add and sub
/// combined.
void ComputeMultConstants64(uint64_t x, uint64_t &add_out, 
                            uint64_t &sub_out);

/// Compute the constants to perform a quicker equivalent of a division of some 
/// 32-bit unsigned integer n by a known constant d (also a 32-bit unsigned 
/// integer).  The constants to compute n/d without explicit division will be 
/// stored in mprime, sh1, and sh2 (unsigned 32-bit integers).
/// 
/// @param d - denominator (divisor)
/// 
/// @param [out] mprime
/// @param [out] sh1
/// @param [out] sh2
void ComputeUDivConstants32(uint32_t d, uint32_t &mprime, uint32_t &sh1, 
                            uint32_t &sh2);

/// Compute the constants to perform a quicker equivalent of a division of some 
/// 32-bit signed integer n by a known constant d (also a 32-bit signed 
/// integer).  The constants to compute n/d without explicit division will be 
/// stored in mprime, dsign, and shpost (signed 32-bit integers).
/// 
/// @param d - denominator (divisor)
/// 
/// @param [out] mprime
/// @param [out] dsign
/// @param [out] shpost
void ComputeSDivConstants32(int32_t d, int32_t &mprime, int32_t &dsign, 
                            int32_t &shpost);

}

#endif /* KLEE_CONSTANTDIVISION_H */
