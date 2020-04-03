//===-- BitArray.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_BITARRAY_H
#define KLEE_BITARRAY_H

namespace klee {

  // XXX would be nice not to have
  // two allocations here for allocated
  // BitArrays
class BitArray {
private:
  uint32_t *bits;
  
protected:
  static uint32_t length(unsigned size) { return (size+31)/32; }

public:
  BitArray(unsigned size, bool value = false) : bits(new uint32_t[length(size)]) {
    memset(bits, value?0xFF:0, sizeof(*bits)*length(size));
  }
  BitArray(const BitArray &b, unsigned size) : bits(new uint32_t[length(size)]) {
    memcpy(bits, b.bits, sizeof(*bits)*length(size));
  }
  ~BitArray() { delete[] bits; }

  bool get(unsigned idx) { return (bool) ((bits[idx/32]>>(idx&0x1F))&1); }
  void set(unsigned idx) { bits[idx/32] |= 1<<(idx&0x1F); }
  void unset(unsigned idx) { bits[idx/32] &= ~(1<<(idx&0x1F)); }
  void set(unsigned idx, bool value) { if (value) set(idx); else unset(idx); }
};

} // End klee namespace

#endif /* KLEE_BITARRAY_H */
