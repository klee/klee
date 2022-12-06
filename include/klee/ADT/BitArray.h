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

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace klee {

// XXX would be nice not to have
// two allocations here for allocated
// BitArrays
class BitArray {
private:
  const unsigned size;
  uint32_t *bits;
  const bool value;

protected:
  static uint32_t length(unsigned size) { return (size + 31) / 32; }

public:
  BitArray(unsigned _size, bool _value = false)
      : size(length(_size)), bits(new uint32_t[size]), value(_value) {
    memset(bits, value ? 0xFF : 0, sizeof(*bits) * size);
  }
  BitArray(const BitArray &b, unsigned _size)
      : size(length(_size)), bits(new uint32_t[size]), value(b.value) {
    /* Fill common part */
    unsigned commonSize = std::min(size, b.size);
    memcpy(bits, b.bits, sizeof(*bits) * commonSize);

    /* Set the remain values to default */
    memset(reinterpret_cast<char *>(bits) + sizeof(*bits) * commonSize,
           value ? 0xFF : 0, sizeof(*bits) * (size - commonSize));
  }
  ~BitArray() { delete[] bits; }

  bool get(unsigned idx) {
    return (bool)((bits[idx / 32] >> (idx & 0x1F)) & 1);
  }
  void set(unsigned idx) { bits[idx / 32] |= 1 << (idx & 0x1F); }
  void unset(unsigned idx) { bits[idx / 32] &= ~(1 << (idx & 0x1F)); }
  void set(unsigned idx, bool value) {
    if (value)
      set(idx);
    else
      unset(idx);
  }
};

} // namespace klee

#endif /* KLEE_BITARRAY_H */
