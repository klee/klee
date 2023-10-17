//===-- slot_allocator.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KDALLOC_SUBALLOCATORS_SLOT_ALLOCATOR_H
#define KDALLOC_SUBALLOCATORS_SLOT_ALLOCATOR_H

#include "../define.h"
#include "../location_info.h"
#include "../tagged_logger.h"

#include "klee/ADT/Bits.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <limits>
#include <ostream>
#include <type_traits>

namespace klee::kdalloc::suballocators {
namespace slotallocator {
template <bool UnlimitedQuarantine> class SlotAllocator;

struct Data final {
  /// The reference count.
  std::size_t referenceCount; // initial value is 1 as soon as this member
                              // is actually allocated

  /// The number of allocated words. Always non-negative.
  std::ptrdiff_t capacity; // initial value is 0 (changes as soon as this
                           // member is actually allocated)

  /// Always less than or equal to the first word that contains a one bit.
  /// Less than or equal to _capacity. Always non-negative.
  std::ptrdiff_t firstFreeFinger; // initial value is 0

  /// Always greater than or equal to the last word that contains a zero bit.
  /// Less than _capacity. May be negative (exactly -1).
  std::ptrdiff_t lastUsedFinger; // initial value is -1

  /// position in the quarantine, followed by the quarantine ring buffer,
  /// followed by the bitmap
  std::size_t quarantineAndBitmap[];
};

class Control final : public TaggedLogger<Control> {
  template <bool UnlimitedQuarantine> friend class SlotAllocator;

  /// pointer to the start of the range managed by this allocator
  char *baseAddress = nullptr;

  /// size in bytes of the range managed by this allocator
  std::size_t size = 0;

  /// size in bytes of the slots that are managed in this slot allocator
  std::size_t slotSize = 0;

  /// number of bytes before the start of the bitmap (includes ordinary
  /// members and quarantine)
  std::size_t prefixSize = -1;

  /// quarantine size *including* the position (=> is never 1)
  std::uint32_t quarantineSize = 0;

  /// true iff the quarantine is unlimited (=> quarantineSize == 0)
  bool unlimitedQuarantine = false;

  [[nodiscard]] inline std::size_t
  convertIndexToPosition(std::size_t index) const noexcept {
    index += 1;
    int const layer =
        std::numeric_limits<std::size_t>::digits - countLeadingZeroes(index);
    auto const layerSlots = static_cast<std::size_t>(1) << (layer - 1);
    auto const currentSlotSize = (size >> layer);
    assert(currentSlotSize > slotSize && "Zero (or below) red zone size!");

    auto const highBit = static_cast<std::size_t>(1) << (layer - 1);
    assert((index & highBit) != 0 && "Failed to compute high bit");
    assert((index ^ highBit) < highBit && "Failed to compute high bit");

    auto layerIndex = index ^ highBit;
    if (layerIndex % 2 == 0) {
      layerIndex /= 2; // drop trailing 0
    } else {
      layerIndex /= 2; // drop trailing 1
      layerIndex = layerSlots - 1 - layerIndex;
    }
    assert(layerIndex < highBit && "Invalid tempering");
    auto const pos = layerIndex * 2 + 1;
    return currentSlotSize * pos;
  }

  [[nodiscard]] inline std::size_t
  convertPositionToIndex(std::size_t const Position) const noexcept {
    int const trailingZeroes = countTrailingZeroes(Position);
    auto const layer = countTrailingZeroes(size) - trailingZeroes;
    auto const layerSlots = static_cast<std::size_t>(1) << (layer - 1);

    auto const highBit = static_cast<std::size_t>(1) << (layer - 1);
    auto layerIndex = Position >> (trailingZeroes + 1);
    assert(layerIndex < highBit && "Tempered value was not restored correctly");

    if (layerIndex < (layerSlots + 1) / 2) {
      layerIndex *= 2; // add trailing 0
    } else {
      layerIndex = layerSlots - 1 - layerIndex;
      layerIndex = layerIndex * 2 + 1; // add trailing 1
    }
    assert(layerIndex < highBit && "Invalid reverse tempering");

    auto const index = highBit ^ layerIndex;
    return index - 1;
  }

public:
  Control() = default;
  Control(Control const &) = delete;
  Control &operator=(Control const &) = delete;
  Control(Control &&) = delete;
  Control &operator=(Control &&) = delete;

  void initialize(void *const baseAddress, std::size_t const size,
                  std::size_t const slotSize, bool const unlimitedQuarantine,
                  std::uint32_t const quarantineSize) noexcept {
    assert(size > 0 && (size & (size - 1)) == 0 &&
           "Sizes of sized bins must be powers of two");

    this->baseAddress = static_cast<char *>(baseAddress);
    this->size = size;
    this->slotSize = slotSize;
    if (unlimitedQuarantine) {
      this->quarantineSize = 0;
      this->unlimitedQuarantine = true;
    } else {
      this->quarantineSize = quarantineSize == 0 ? 0 : quarantineSize + 1;
      this->unlimitedQuarantine = false;
    }
    this->prefixSize =
        sizeof(Data) + this->quarantineSize * sizeof(std::size_t);

    traceLine("Initialization complete");
  }

  inline std::ostream &logTag(std::ostream &out) const noexcept {
    return out << "[slot " << slotSize << " Control] ";
  }

  bool isQuarantineUnlimited() const noexcept { return unlimitedQuarantine; }

  constexpr void *mapping_begin() const noexcept { return baseAddress; }
  constexpr void *mapping_end() const noexcept {
    return static_cast<void *>(static_cast<char *>(baseAddress) + size);
  }
};

template <>
class SlotAllocator<false> final : public TaggedLogger<SlotAllocator<false>> {
  static_assert(static_cast<std::size_t>(-1) == ~static_cast<std::size_t>(0),
                "-1 must be ~0 for size_t");

  static_assert(std::is_trivial<Data>::value &&
                    std::is_standard_layout<Data>::value,
                "Data must be POD");

  Data *data = nullptr;

  inline void releaseData() noexcept {
    if (data) {
      --data->referenceCount;
      if (data->referenceCount == 0) {
        std::free(data);
      }
      data = nullptr;
    }
  }

  inline void acquireData(Control const &Control) noexcept {
    assert(!!data);
    if (data->referenceCount > 1) {
      auto newCapacity = computeNextCapacity(
          getLastUsed(Control) +
          1); // one more, since `getLastUsed` is an index, not a size
      auto objectSize = Control.prefixSize + newCapacity * sizeof(std::size_t);
      auto newData = static_cast<Data *>(std::malloc(objectSize));
      assert(newData && "allocation failure");

      std::memcpy(newData, data,
                  static_cast<std::size_t>(
                      reinterpret_cast<char *>(
                          &data->quarantineAndBitmap[Control.quarantineSize +
                                                     data->lastUsedFinger] +
                          1) -
                      reinterpret_cast<char *>(data)));
      newData->referenceCount = 1;
      newData->capacity = newCapacity;
      std::fill(
          &newData->quarantineAndBitmap[Control.quarantineSize +
                                        newData->lastUsedFinger + 1],
          &newData->quarantineAndBitmap[Control.quarantineSize + newCapacity],
          ~static_cast<std::size_t>(0));

      --data->referenceCount;
      assert(data->referenceCount > 0);
      data = newData;
    }
    assert(data->referenceCount == 1);
  }

  std::size_t quarantine(Control const &control, std::size_t const index) {
    assert(!!data &&
           "Deallocations can only happen if allocations happened beforehand");

    if (control.quarantineSize == 0) {
      return index;
    }

    assert(data->referenceCount == 1 &&
           "Must hold CoW ownership to quarantine a new index");

    auto const pos = data->quarantineAndBitmap[0];
    if (pos + 1 == control.quarantineSize) {
      data->quarantineAndBitmap[0] = 1;
    } else {
      data->quarantineAndBitmap[0] = pos + 1;
    }

    return std::exchange(data->quarantineAndBitmap[pos], index);
  }

  inline static constexpr std::ptrdiff_t
  computeNextCapacity(std::ptrdiff_t oldCapacity) noexcept {
    assert(oldCapacity >= 0);
    assert(oldCapacity <
           (std::numeric_limits<std::ptrdiff_t>::max)() /
               std::max<std::ptrdiff_t>(8, sizeof(std::uint64_t)));
    return std::max<std::ptrdiff_t>(8, (oldCapacity * 7) / 4);
  }

  /// Get index of first word that contains a one bit. (Internally update the
  /// associated finger.)
  [[nodiscard]] std::ptrdiff_t
  getFirstFree(Control const &control) const noexcept {
    if (!data) {
      return 0;
    }

    assert(data->firstFreeFinger >= 0);
    assert(data->firstFreeFinger <= data->capacity);

    while (data->firstFreeFinger < data->capacity &&
           data->quarantineAndBitmap[control.quarantineSize +
                                     data->firstFreeFinger] == 0) {
      ++data->firstFreeFinger;
    }
    assert(data->firstFreeFinger <= data->capacity);

    return data->firstFreeFinger;
  }

  /// Get index of last word that contains a zero bit. (Internally update the
  /// associated finger.)
  [[nodiscard]] inline std::ptrdiff_t
  getLastUsed(Control const &control) const noexcept {
    if (!data) {
      return -1;
    }

    assert(data->lastUsedFinger >= -1);
    assert(data->lastUsedFinger < data->capacity);

    while (data->lastUsedFinger >= 0 &&
           ~data->quarantineAndBitmap[control.quarantineSize +
                                      data->lastUsedFinger] == 0) {
      --data->lastUsedFinger;
    }
    assert(data->lastUsedFinger >= -1);

    return data->lastUsedFinger;
  }

  /// Returns true iff the bit at `index` is used
  [[nodiscard]] bool isUsed(Control const &control,
                            std::size_t const index) const noexcept {
    if (!data) {
      return false;
    }

    auto const loc = static_cast<std::ptrdiff_t>(
        index / std::numeric_limits<std::size_t>::digits);
    auto const shift =
        static_cast<int>(index % std::numeric_limits<std::size_t>::digits);

    if (loc <= data->lastUsedFinger) {
      return (data->quarantineAndBitmap[control.quarantineSize + loc] &
              (static_cast<std::uint64_t>(1) << shift)) == 0;
    } else {
      return false;
    }
  }

  void setFree(Control const &control, std::size_t const index) noexcept {
    auto const loc = static_cast<std::ptrdiff_t>(
        index / std::numeric_limits<std::size_t>::digits);
    auto const shift =
        static_cast<int>(index % std::numeric_limits<std::size_t>::digits);

    assert(!!data);
    assert(loc <= data->lastUsedFinger);
    assert(data->lastUsedFinger < data->capacity);
    // 0 <= loc <= _last_used_finger < _capacity

    acquireData(control);

    auto word = data->quarantineAndBitmap[control.quarantineSize + loc];
    auto const mask = static_cast<std::size_t>(1) << shift;
    assert((word & mask) == 0);
    word ^= mask;
    data->quarantineAndBitmap[control.quarantineSize + loc] = word;

    if (~word == 0 && loc == data->lastUsedFinger) {
      data->lastUsedFinger = loc - 1;
    }

    if (loc < data->firstFreeFinger) {
      data->firstFreeFinger = loc;
    }
  }

  /// Toggles the first free bit to allocated and returns its index
  [[nodiscard]] std::size_t
  setFirstFreeToUsed(Control const &control) noexcept {
    auto const loc = getFirstFree(control);
    if (!data) {
      auto newCapacity = computeNextCapacity(0);
      auto objectSize = control.prefixSize + newCapacity * sizeof(std::size_t);
      data = static_cast<Data *>(std::malloc(objectSize));
      assert(data && "allocation failure");
      data->referenceCount = 1;
      data->capacity = newCapacity;
      data->firstFreeFinger = 0;
      data->lastUsedFinger = -1;

      if (control.quarantineSize == 0) {
        std::fill(&data->quarantineAndBitmap[0],
                  &data->quarantineAndBitmap[newCapacity],
                  ~static_cast<std::size_t>(0));
      } else {
        data->quarantineAndBitmap[0] = 1;
        std::fill(
            &data->quarantineAndBitmap[1],
            &data->quarantineAndBitmap[control.quarantineSize + newCapacity],
            ~static_cast<std::size_t>(0));
      }
    } else {
      if (loc == data->capacity && data->referenceCount == 1) {
        auto newCapacity = computeNextCapacity(data->capacity);
        auto objectSize =
            control.prefixSize + newCapacity * sizeof(std::size_t);
        data = static_cast<Data *>(std::realloc(data, objectSize));
        assert(data && "allocation failure");
        std::fill(
            &data->quarantineAndBitmap[control.quarantineSize + data->capacity],
            &data->quarantineAndBitmap[control.quarantineSize + newCapacity],
            ~static_cast<std::size_t>(0));
        data->capacity = newCapacity;
      } else {
        acquireData(control);
        assert(loc < data->capacity &&
               "acquire_data performs a growth step in the sense of "
               "`computeNextCapacity`");
      }
    }

    assert(!!data);
    assert(data->referenceCount == 1);
    assert(loc < data->capacity);

    auto word = data->quarantineAndBitmap[control.quarantineSize + loc];
    auto const shift = countTrailingZeroes(word);
    auto const mask = static_cast<std::size_t>(1) << shift;
    assert((word & mask) == mask);
    word ^= mask;
    data->quarantineAndBitmap[control.quarantineSize + loc] = word;

    if (word == 0 && data->firstFreeFinger == loc) {
      data->firstFreeFinger = loc + 1;
    }

    if (loc > data->lastUsedFinger) {
      data->lastUsedFinger = loc;
    }

    return static_cast<std::size_t>(
        loc * std::numeric_limits<std::size_t>::digits + shift);
  }

public:
  constexpr SlotAllocator() = default;

  SlotAllocator(SlotAllocator const &rhs) noexcept : data(rhs.data) {
    if (data) {
      ++data->referenceCount;
      assert(data->referenceCount > 1);
    }
  }

  SlotAllocator &operator=(SlotAllocator const &rhs) noexcept {
    if (data != rhs.data) {
      releaseData();
      data = rhs.data;
      if (data) {
        ++data->referenceCount;
        assert(data->referenceCount > 1);
      }
    }
    return *this;
  }

  SlotAllocator(SlotAllocator &&rhs) noexcept
      : data(std::exchange(rhs.data, nullptr)) {
    assert(data == nullptr || data->referenceCount > 0);
  }

  SlotAllocator &operator=(SlotAllocator &&rhs) noexcept {
    if (data != rhs.data) {
      releaseData();
      data = std::exchange(rhs.data, nullptr);
    }
    return *this;
  }

  ~SlotAllocator() noexcept { releaseData(); }

  inline std::ostream &logTag(std::ostream &out) const noexcept {
    return out << "[slot] ";
  }

  LocationInfo getLocationInfo(Control const &control, void const *const ptr,
                               std::size_t const size) const noexcept {
    assert(control.mapping_begin() <= ptr &&
           reinterpret_cast<char const *>(ptr) + size < control.mapping_end() &&
           "This property should have been ensured by the caller");

    auto const begin = static_cast<std::size_t>(static_cast<char const *>(ptr) -
                                                control.baseAddress);
    auto const end = static_cast<std::size_t>(static_cast<char const *>(ptr) +
                                              size - control.baseAddress);
    assert(control.slotSize > 0 && "Uninitialized Control structure");
    auto const pos = begin - begin % control.slotSize;
    if (pos != (end - 1) - (end - 1) % control.slotSize) {
      return LocationInfo::LI_Unaligned;
    }

    auto const index = control.convertPositionToIndex(pos);
    auto const loc = static_cast<std::ptrdiff_t>(
        index / std::numeric_limits<std::size_t>::digits);
    auto const shift =
        static_cast<int>(index % std::numeric_limits<std::size_t>::digits);

    if (!data || loc > data->lastUsedFinger) {
      return LocationInfo::LI_Unallocated;
    }
    assert(data->lastUsedFinger < data->capacity);

    auto word = data->quarantineAndBitmap[control.quarantineSize + loc];
    auto const mask = static_cast<std::size_t>(1) << shift;
    if ((word & mask) == 0) {
      return {LocationInfo::LI_AllocatedOrQuarantined,
              control.baseAddress + pos};
    } else {
      return LocationInfo::LI_Unallocated;
    }
  }

  [[nodiscard]] void *allocate(Control const &control) noexcept {
    traceLine("Allocating ", control.slotSize, " bytes");
    traceContents(control);

    auto const index = setFirstFreeToUsed(control);
    return control.baseAddress + control.convertIndexToPosition(index);
  }

  void deallocate(Control const &control, void *const ptr) {
    assert(!!data &&
           "Deallocations can only happen if allocations happened beforehand");

    auto pos = static_cast<std::size_t>(static_cast<char *>(ptr) -
                                        control.baseAddress);
    acquireData(control); // we will need quarantine and/or bitmap ownership

    traceLine("Quarantining ", ptr, " as ", pos, " for ",
              control.quarantineSize, " deallocations");
    pos = quarantine(control, pos);

    if (pos == static_cast<std::size_t>(-1)) {
      traceLine("Nothing to deallocate");
    } else {
      traceLine("Deallocating ", pos);
      traceContents(control);
      assert(pos < control.size);

      setFree(control, control.convertPositionToIndex(pos));
    }
  }

  void traceContents(Control const &control) const noexcept {
    static_cast<void>(control);

#if KDALLOC_TRACE >= 2
    traceLine("bitmap:");
    bool isEmpty = true;
    for (std::size_t i = 0; i < data->capacity; ++i) {
      if (is_used(Control, i)) {
        isEmpty = false;
        traceLine("  ", i, " ",
                  static_cast<void *>(control.baseAddress +
                                      control.convertIndexToPosition(i)));
      }
    }
    if (isEmpty) {
      traceLine("  <empty>");
    }
#else
    traceLine("bitmap has a capacity of ", data ? data->capacity : 0,
              " entries");
#endif
  }
};

template <>
class SlotAllocator<true> final : public TaggedLogger<SlotAllocator<true>> {
  std::size_t next;

public:
  inline std::ostream &logTag(std::ostream &out) const noexcept {
    return out << "[uslot] ";
  }

  LocationInfo getLocationInfo(Control const &control, void const *const ptr,
                               std::size_t const size) const noexcept {
    assert(control.mapping_begin() <= ptr &&
           reinterpret_cast<char const *>(ptr) + size < control.mapping_end() &&
           "This property should have been ensured by the caller");

    auto const begin = static_cast<std::size_t>(static_cast<char const *>(ptr) -
                                                control.baseAddress);
    auto const end = static_cast<std::size_t>(static_cast<char const *>(ptr) +
                                              size - control.baseAddress);
    assert(control.slotSize > 0 && "Uninitialized Control structure");
    auto const pos = begin - begin % control.slotSize;
    if (pos != (end - 1) - (end - 1) % control.slotSize) {
      return LocationInfo::LI_Unaligned;
    }

    auto const index = control.convertPositionToIndex(pos);
    if (index >= next) {
      return LocationInfo::LI_Unallocated;
    } else {
      return {LocationInfo::LI_AllocatedOrQuarantined,
              control.baseAddress + pos};
    }
  }

  [[nodiscard]] void *allocate(Control const &control) noexcept {
    traceLine("Allocating ", control.slotSize, " bytes");

    return control.baseAddress + control.convertIndexToPosition(next++);
  }

  void deallocate(Control const &control, void *const ptr) {
    traceLine("Quarantining ", ptr, " for ever");
  }

  void traceContents(Control const &) const noexcept {
    traceLine("next: ", next);
  }
};
} // namespace slotallocator

using slotallocator::SlotAllocator;
using SlotAllocatorControl = slotallocator::Control;
} // namespace klee::kdalloc::suballocators

#endif
