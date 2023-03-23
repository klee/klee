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

#include <cstdint>
#include <cstring>
#include <memory>

namespace klee {
class BitArray {
private:
  std::unique_ptr<std::uint32_t[]> bits;

protected:
  static std::uint32_t length(unsigned size) { return (size + 31) / 32; }

public:
  BitArray(unsigned size, bool value = false)
      : bits(new std::uint32_t[length(size)]) {
    std::memset(bits.get(), value ? 0xFF : 0, sizeof(bits[0]) * length(size));
  }
  BitArray(const BitArray &b, unsigned size)
      : bits(new std::uint32_t[length(size)]) {
    std::memcpy(bits.get(), b.bits.get(), sizeof(bits[0]) * length(size));
  }

  bool get(unsigned idx) {
    return static_cast<bool>((bits[idx / 32] >> (idx & 0x1F)) & 1);
  }
  void set(unsigned idx) { bits[idx / 32] |= 1 << (idx & 0x1F); }
  void unset(unsigned idx) { bits[idx / 32] &= ~(1 << (idx & 0x1F)); }
  void set(unsigned idx, bool value) {
    if (value) {
      set(idx);
    } else {
      unset(idx);
    }
  }
};
} // namespace klee

#endif /* KLEE_BITARRAY_H */
