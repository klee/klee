//===-- loh.h ---------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KDALLOC_SUBALLOCATORS_LOH_H
#define KDALLOC_SUBALLOCATORS_LOH_H

#include "../define.h"
#include "../location_info.h"
#include "../tagged_logger.h"
#include "sized_regions.h"

#include "klee/ADT/Bits.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <ostream>
#include <utility>
#include <vector>

#if KDALLOC_TRACE >= 2
#include <iostream>
#endif

namespace klee::kdalloc::suballocators {
/// The large object heap is implemented as what amounts to as a bi-directional
/// mapping between the position of each unallocated region and its actual size.
/// The implemented algorithm performs allocations in the middle of the largest
/// available unallocated region. Allocations are guaranteed to be aligned to
/// 4096 bytes.
class LargeObjectAllocator final : public TaggedLogger<LargeObjectAllocator> {
  struct Data final {
    /// The reference count.
    std::size_t referenceCount = 1;

    /// A CoW enabled treap that is a tree over base addresses and a max-heap
    /// over sizes
    SizedRegions regions;

    static_assert(sizeof(void *) >= sizeof(std::size_t),
                  "The quarantine structure contains a `std::size_t` followed "
                  "by many `void*`s");
    static_assert(alignof(void *) >= alignof(std::size_t),
                  "The quarantine structure contains a `std::size_t` followed "
                  "by many `void*`s");
    using QuarantineElement =
        std::aligned_storage_t<sizeof(void *), alignof(void *)>;

    /// The quarantine position followed by the quarantine ring buffer
    /// Structure: [pos, ptr1, ptr2, ...]
    QuarantineElement quarantine[];

    Data() = default;
    Data(Data const &rhs) : regions(rhs.regions) {}
    Data &operator=(Data const &) = delete;
    Data(Data &&) = delete;
    Data &operator=(Data &&) = delete;
  };

public:
  class Control final : public TaggedLogger<Control> {
    friend class LargeObjectAllocator;

    void *baseAddress = nullptr;
    std::size_t size = 0;
    std::uint32_t quarantineSize = 0;
    bool unlimitedQuarantine = false;

  public:
    Control() = default;
    Control(Control const &) = delete;
    Control &operator=(Control const &) = delete;
    Control(Control &&) = delete;
    Control &operator=(Control &&) = delete;

    void initialize(void *const baseAddress, std::size_t const size,
                    bool const unlimitedQuarantine,
                    std::uint32_t const quarantineSize) {
      assert(size >= 3 * 4096 && "The LOH requires at least three pages");

      this->baseAddress = baseAddress;
      this->size = size;
      if (unlimitedQuarantine) {
        this->quarantineSize = 0;
        this->unlimitedQuarantine = true;
      } else {
        this->quarantineSize = quarantineSize == 0 ? 0 : quarantineSize + 1;
        this->unlimitedQuarantine = false;
      }

      traceLine("Initialization complete for region ", baseAddress, " + ",
                size);
    }

    inline std::ostream &logTag(std::ostream &out) const noexcept {
      return out << "[LOH control] ";
    }

    constexpr void *mapping_begin() const noexcept { return baseAddress; }
    constexpr void *mapping_end() const noexcept {
      return static_cast<void *>(static_cast<char *>(baseAddress) + size);
    }
  };

private:
  Data *data = nullptr;

  inline void releaseData() noexcept {
    if (data) {
      --data->referenceCount;
      if (data->referenceCount == 0) {
        data->~Data();
        std::free(data);
      }
      data = nullptr;
    }
  }

  inline void acquireData(Control const &control) noexcept {
    assert(!!data);
    if (data->referenceCount > 1) {
      auto newData = static_cast<Data *>(std::malloc(
          sizeof(Data) + control.quarantineSize * sizeof(std::size_t)));
      assert(newData && "allocation failure");

      new (newData) Data(*data);
      std::memcpy(&newData->quarantine[0], &data->quarantine[0],
                  sizeof(Data::QuarantineElement) * control.quarantineSize);

      --data->referenceCount;
      assert(data->referenceCount > 0);
      data = newData;
    }
    assert(data->referenceCount == 1);
  }

  void *quarantine(Control const &control, void *const ptr) {
    assert(!!data &&
           "Deallocations can only happen if allocations happened beforehand");
    assert(!control.unlimitedQuarantine);

    if (control.quarantineSize == 0) {
      return ptr;
    }

    assert(data->referenceCount == 1 &&
           "Must hold CoW ownership to quarantine a new pointer+size pair");

    auto const pos = reinterpret_cast<std::size_t &>(data->quarantine[0]);
    if (pos + 1 == control.quarantineSize) {
      reinterpret_cast<std::size_t &>(data->quarantine[0]) = 1;
    } else {
      reinterpret_cast<std::size_t &>(data->quarantine[0]) = pos + 1;
    }

    return std::exchange(reinterpret_cast<void *&>(data->quarantine[pos]),
                         std::move(ptr));
  }

public:
  constexpr LargeObjectAllocator() = default;

  LargeObjectAllocator(LargeObjectAllocator const &rhs) noexcept
      : data(rhs.data) {
    if (data) {
      ++data->referenceCount;
      assert(data->referenceCount > 1);
    }
  }

  LargeObjectAllocator &operator=(LargeObjectAllocator const &rhs) noexcept {
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

  LargeObjectAllocator(LargeObjectAllocator &&rhs) noexcept
      : data(std::exchange(rhs.data, nullptr)) {
    if (data) {
      assert(data->referenceCount > 0);
    }
  }

  LargeObjectAllocator &operator=(LargeObjectAllocator &&rhs) noexcept {
    if (data != rhs.data) {
      releaseData();
      data = std::exchange(rhs.data, nullptr);
    }
    return *this;
  }

  ~LargeObjectAllocator() noexcept { releaseData(); }

  inline std::ostream &logTag(std::ostream &out) const noexcept {
    return out << "[LOH] ";
  }

  std::size_t getSize(Control const &control,
                      void const *const ptr) const noexcept {
    assert(control.mapping_begin() <= ptr && ptr < control.mapping_end() &&
           "This property should have been ensured by the caller");
    assert(!!data &&
           "Can only get size of an object if objects already exist...");

    return data->regions.getSize(static_cast<char const *>(ptr));
  }

  LocationInfo getLocationInfo(Control const &control, void const *const ptr,
                               std::size_t const size) const noexcept {
    assert(control.mapping_begin() <= ptr &&
           reinterpret_cast<char const *>(ptr) + size < control.mapping_end() &&
           "This property should have been ensured by the caller");

    if (!data) {
      return LocationInfo::LI_Unallocated;
    }
    assert(!data->regions.isEmpty());

    if (reinterpret_cast<char const *>(control.mapping_begin()) + 4096 >
            reinterpret_cast<char const *>(ptr) ||
        reinterpret_cast<char const *>(control.mapping_end()) - 4096 <
            reinterpret_cast<char const *>(ptr) + size) {
      return LocationInfo::LI_AlwaysRedzone;
    }

    return data->regions.getLocationInfo(reinterpret_cast<char const *>(ptr),
                                         size);
  }

  [[nodiscard]] void *allocate(Control const &control, std::size_t size) {
    if (!data) {
      data = static_cast<Data *>(std::malloc(
          sizeof(Data) + control.quarantineSize * sizeof(std::size_t)));
      assert(data && "allocation failure");

      new (data) Data();
      if (control.quarantineSize > 0) {
        reinterpret_cast<std::size_t &>(data->quarantine[0]) = 1;
        std::memset(&data->quarantine[1], 0,
                    (control.quarantineSize - 1) *
                        sizeof(Data::QuarantineElement));
      }

      data->regions.insert(static_cast<char *>(control.baseAddress),
                           control.size);
    } else {
      acquireData(control);
    }
    assert(data->referenceCount == 1);

    auto const quantizedSize = roundUpToMultipleOf4096(size);
    traceLine("Allocating ", size, " (", quantizedSize, ") bytes");
    assert(size > 0);
    assert(quantizedSize % 4096 == 0);
    traceContents(control);

    size = quantizedSize;

    assert(!data->regions.isEmpty());
    auto const &node = data->regions.getLargestRegion();
    std::size_t const rangeSize = node->getSize();
    assert(rangeSize + 2 * 4096 >= size && "Zero (or below) red zone size!");
    char *const rangePos = node->getBaseAddress();

    auto offset = (rangeSize - size) / 2;
    offset = roundUpToMultipleOf4096(offset);

    // left subrange
    data->regions.resizeLargestRegion(offset);

    // right subrange
    data->regions.insert(rangePos + offset + size, rangeSize - offset - size);

    auto const result = static_cast<void *>(rangePos + offset);
    traceLine("Allocation result: ", result);
    return result;
  }

  void deallocate(Control const &control, void *ptr) {
    assert(!!data &&
           "Deallocations can only happen if allocations happened beforehand");

    if (control.unlimitedQuarantine) {
      traceLine("Quarantining ", ptr, " for ever");
    } else {
      traceLine("Quarantining ", ptr, " for ", control.quarantineSize - 1,
                " deallocations");

      acquireData(control); // we will need quarantine and/or region ownership
      ptr = quarantine(control, ptr);
      if (!ptr) {
        return;
      }

      traceLine("Deallocating ", ptr);

      assert(data->referenceCount == 1);
      traceContents(control);
      traceLine("Merging regions around ",
                static_cast<void *>(static_cast<char *>(ptr)));

      data->regions.mergeAroundAddress(static_cast<char *>(ptr));
    }
  }

  void traceContents(Control const &) const noexcept {
    if (data->regions.isEmpty()) {
      traceLine("regions is empty");
    } else {
#if KDALLOC_TRACE >= 2
      traceLine("regions:");
      data->regions.dump(std::cout);

#if !defined(NDEBUG)
      auto invariantsHold = data->regions.checkInvariants();
      if (!invariantsHold.first) {
        traceln("TREE INVARIANT DOES NOT HOLD!");
      }
      if (!invariantsHold.second) {
        traceln("HEAP INVARIANT DOES NOT HOLD!");
      }
      assert(invariantsHold.first && invariantsHold.second);
#endif
#else
      traceLine("regions is not empty");
#endif
    }
  }
};
} // namespace klee::kdalloc::suballocators

#endif
