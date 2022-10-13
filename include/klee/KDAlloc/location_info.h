//===-- location_info.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KDALLOC_LOCATION_INFO_H
#define KDALLOC_LOCATION_INFO_H

#include <cassert>

namespace klee::kdalloc {
class LocationInfo {
public:
  enum Enum {
    /// refers to the null page
    LI_NullPage,
    /// location is not inside the mapping (but not on the null page)
    LI_NonNullOutsideMapping,
    /// location spans suballocator mapping boundary
    LI_SpansSuballocators,
    /// area spans object alignment boundary
    LI_Unaligned,
    /// location always refers to a redzone
    LI_AlwaysRedzone,
    /// location currently refers to a redzone
    LI_CurrentRedzone,
    /// location is inside an object that is either currently allocated or in
    /// quarantine
    LI_AllocatedOrQuarantined,
    /// location is potentially valid, but not currently allocated
    LI_Unallocated,
  };

private:
  Enum value;
  void *address;

public:
  constexpr LocationInfo(Enum value, void *address = nullptr) noexcept
      : value(value), address(address) {}
  constexpr operator Enum() noexcept { return value; }

  /// location is (partially) outside the mapping
  constexpr bool isOutsideMapping() const noexcept {
    return value != Enum::LI_NullPage &&
           value != Enum::LI_NonNullOutsideMapping;
  }

  /// location is potentially valid
  constexpr bool isValid() const noexcept {
    return value == Enum::LI_AllocatedOrQuarantined ||
           value == Enum::LI_Unallocated || value == Enum::LI_CurrentRedzone;
  }

  /// location (partially) refers to a redzone
  constexpr bool isRedzone() const noexcept {
    return value == Enum::LI_AlwaysRedzone || value == Enum::LI_CurrentRedzone;
  }

  constexpr void *getBaseAddress() const noexcept {
    assert(value == Enum::LI_AllocatedOrQuarantined);
    return address;
  }
};
} // namespace klee::kdalloc

#endif
