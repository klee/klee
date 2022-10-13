//===-- allocator.h ---------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KDALLOC_ALLOCATOR_H
#define KDALLOC_ALLOCATOR_H

#include "define.h"
#include "location_info.h"
#include "mapping.h"
#include "suballocators/loh.h"
#include "suballocators/slot_allocator.h"
#include "tagged_logger.h"

#include "klee/ADT/Bits.h"
#include "klee/ADT/Ref.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <type_traits>

namespace klee::kdalloc {
/// Wraps a mapping that is shared with other allocators.
class Allocator final : public TaggedLogger<Allocator> {
public:
  class Control final {
    friend class Allocator;
    friend class AllocatorFactory;

    static constexpr const std::uint32_t unlimitedQuarantine =
        static_cast<std::uint32_t>(-1);

    /// @todo This should really be a data member `static constexpr const
    /// std::array meta = { ... }`.
    static inline const std::array<std::size_t, 8> &getMeta() noexcept {
      static const std::array<std::size_t, 8> meta = {
          1u,    // bool
          4u,    // int
          8u,    // pointer size
          16u,   // double
          32u,   // compound types #1
          64u,   // compound types #2
          256u,  // compound types #3
          2048u, // reasonable buffers
      };
      return meta;
    }

    [[nodiscard]] static inline int
    convertSizeToBinIndex(std::size_t const size) noexcept {
      for (std::size_t i = 0; i < getMeta().size(); ++i) {
        if (getMeta()[i] >= size) {
          return i;
        }
      }
      return getMeta().size();
    }

  public:
    mutable class klee::ReferenceCounter _refCount;

  private:
    Mapping mapping;
    std::array<suballocators::SlotAllocator::Control,
               std::tuple_size<std::decay_t<decltype(getMeta())>>::value>
        sizedBins;
    suballocators::LargeObjectAllocator::Control largeObjectBin;

  public:
    Control() = delete;
    Control(Control const &) = delete;
    Control &operator=(Control const &) = delete;
    Control(Control &&) = delete;
    Control &operator=(Control &&) = delete;

  private:
    Control(Mapping &&mapping) : mapping(std::move(mapping)) {}
  };

  static constexpr const auto unlimitedQuarantine =
      Control::unlimitedQuarantine;

private:
  klee::ref<Control> control;

  std::array<suballocators::SlotAllocator,
             std::tuple_size<std::decay_t<decltype(Control::getMeta())>>::value>
      sizedBins;
  suballocators::LargeObjectAllocator largeObjectBin;

public:
  std::ostream &logTag(std::ostream &out) const noexcept {
    return out << "[alloc] ";
  }

  Allocator() = default;
  Allocator(Allocator const &rhs) = default;
  Allocator &operator=(Allocator const &rhs) = default;
  Allocator(Allocator &&) = default;
  Allocator &operator=(Allocator &&) = default;

  Allocator(klee::ref<Control> control) : control(std::move(control)) {}

  explicit operator bool() const noexcept { return !control.isNull(); }

  auto const &getSizedBinInfo() const noexcept { return Control::getMeta(); }

  [[nodiscard]] void *allocate(std::size_t size) {
    assert(*this && "Invalid allocator");

    auto const bin = Control::convertSizeToBinIndex(size);
    traceLine("Allocating ", size, " bytes in bin ", bin);

    void *result = nullptr;
    if (bin < static_cast<int>(sizedBins.size())) {
      result = sizedBins[bin].allocate(control->sizedBins[bin]);
    } else {
      result = largeObjectBin.allocate(control->largeObjectBin, size);
    }
    traceLine("Allocated ", result);
    return result;
  }

  void free(void *ptr, std::size_t size) {
    assert(*this && "Invalid allocator");
    assert(ptr && "Freeing nullptrs is not supported"); // we are not ::free!

    auto const bin = Control::convertSizeToBinIndex(size);
    traceLine("Freeing ", ptr, " of size ", size, " in bin ", bin);

    if (bin < static_cast<int>(sizedBins.size())) {
      return sizedBins[bin].deallocate(control->sizedBins[bin], ptr);
    } else {
      return largeObjectBin.deallocate(control->largeObjectBin, ptr, size);
    }
  }

  LocationInfo location_info(void const *const ptr,
                             std::size_t const size) const noexcept {
    assert(*this && "Invalid allocator");

    if (!ptr || reinterpret_cast<std::uintptr_t>(ptr) < 4096) {
      return LocationInfo::LI_NullPage;
    }

    // the following is technically UB if `ptr` does not actually point inside
    // the mapping at all
    for (std::size_t i = 0; i < Allocator::Control::getMeta().size(); ++i) {
      if (control->sizedBins[i].mapping_begin() <= ptr &&
          ptr < control->sizedBins[i].mapping_end()) {
        if (reinterpret_cast<char const *>(ptr) + size <=
            control->sizedBins[i].mapping_end()) {
          return sizedBins[i].getLocationInfo(control->sizedBins[i], ptr, size);
        } else {
          return LocationInfo::LI_SpansSuballocators;
        }
      }
    }
    if (control->largeObjectBin.mapping_begin() <= ptr &&
        ptr < control->largeObjectBin.mapping_end()) {
      if (reinterpret_cast<char const *>(ptr) + size <=
          control->largeObjectBin.mapping_end()) {
        return largeObjectBin.getLocationInfo(control->largeObjectBin, ptr,
                                              size);
      } else {
        // the loh is the suballocator using the largest addresses, therefore
        // its mapping is not followed by that of another suballocator, but
        // rather the end of the mapping
        return LocationInfo::LI_NonNullOutsideMapping;
      }
    }

    return LocationInfo::LI_NonNullOutsideMapping;
  }
};

class AllocatorFactory {
public:
  static constexpr const auto unlimitedQuarantine =
      Allocator::Control::unlimitedQuarantine;

private:
  klee::ref<Allocator::Control> control;

public:
  AllocatorFactory() = default;

  AllocatorFactory(std::size_t const size, std::uint32_t const quarantineSize)
      : AllocatorFactory(Mapping{0, size}, quarantineSize) {}

  AllocatorFactory(std::uintptr_t const address, std::size_t const size,
                   std::uint32_t const quarantineSize)
      : AllocatorFactory(Mapping{address, size}, quarantineSize) {}

  AllocatorFactory(Mapping &&mapping, std::uint32_t const quarantineSize) {
    assert(mapping && "Invalid mapping");
    assert(mapping.getSize() >
               Allocator::Control::getMeta().size() * 4096 + 3 * 4096 &&
           "Mapping is *far* to small");

    control = new Allocator::Control(std::move(mapping));
    auto const binSize =
        static_cast<std::size_t>(1)
        << (std::numeric_limits<std::size_t>::digits - 1 -
            countLeadingZeroes(control->mapping.getSize() /
                               (Allocator::Control::getMeta().size() + 1)));
    char *const base = static_cast<char *>(control->mapping.getBaseAddress());
    std::size_t totalSize = 0;
    for (std::size_t i = 0; i < Allocator::Control::getMeta().size(); ++i) {
      control->sizedBins[i].initialize(
          base + totalSize, binSize, Allocator::Control::getMeta()[i],
          quarantineSize == unlimitedQuarantine,
          quarantineSize == unlimitedQuarantine ? 0 : quarantineSize);

      totalSize += binSize;
      assert(totalSize <= control->mapping.getSize() && "Mapping too small");
    }

    auto largeObjectBinSize = control->mapping.getSize() - totalSize;
    assert(largeObjectBinSize > 0);
    control->largeObjectBin.initialize(
        base + totalSize, largeObjectBinSize,
        quarantineSize == unlimitedQuarantine,
        quarantineSize == unlimitedQuarantine ? 0 : quarantineSize);
  }

  explicit operator bool() const noexcept { return !control.isNull(); }

  Mapping &getMapping() noexcept {
    assert(!!*this && "Cannot get mapping of uninitialized factory.");
    return control->mapping;
  }

  Mapping const &getMapping() const noexcept {
    assert(!!*this && "Cannot get mapping of uninitialized factory.");
    return control->mapping;
  }

  Allocator makeAllocator() const {
    assert(!!*this &&
           "Can only create an allocator from an initialized factory.");

    return Allocator{control};
  }

  explicit operator Allocator() && {
    assert(!!*this &&
           "Can only create an allocator from an initialized factory.");

    return Allocator{std::move(control)};
  }
};
} // namespace klee::kdalloc

#endif
