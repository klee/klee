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
/// Wraps a mapping and delegates allocation to one of 8 sized-bin slot
/// allocators (size < 4096) or a large object allocator (size >= 4096).
class Allocator final : public TaggedLogger<Allocator> {
public:
  class Control final {
    friend class Allocator;
    friend class AllocatorFactory;

    static constexpr const std::uint32_t unlimitedQuarantine =
        static_cast<std::uint32_t>(-1);

    static constexpr const std::array<std::size_t, 8> meta = {
        1u,    // bool
        4u,    // int
        8u,    // pointer size
        16u,   // double
        32u,   // compound types #1
        64u,   // compound types #2
        256u,  // compound types #3
        2048u, // reasonable buffers
    };

    [[nodiscard]] static inline int
    convertSizeToBinIndex(std::size_t const size) noexcept {
      for (std::size_t i = 0; i < meta.size(); ++i) {
        if (meta[i] >= size) {
          return i;
        }
      }
      return meta.size();
    }

    [[nodiscard]] inline int
    convertPtrToBinIndex(void const *const p) noexcept {
      for (std::size_t i = 0; i < sizedBins.size(); ++i) {
        if (p >= sizedBins[i].mapping_begin() &&
            p < sizedBins[i].mapping_end()) {
          return i;
        }
      }
      assert(p >= largeObjectBin.mapping_begin() &&
             p < largeObjectBin.mapping_end());
      return meta.size();
    }

  public:
    mutable class klee::ReferenceCounter _refCount;

  private:
    Mapping mapping;
    std::array<suballocators::SlotAllocatorControl, meta.size()> sizedBins;
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

  std::array<std::aligned_union_t<0, suballocators::SlotAllocator<false>,
                                  suballocators::SlotAllocator<true>>,
             Control::meta.size()>
      sizedBins;
  suballocators::LargeObjectAllocator largeObjectBin;

  void initializeSizedBins() {
    if (control) {
      for (std::size_t i = 0; i < sizedBins.size(); ++i) {
        if (control->sizedBins[i].isQuarantineUnlimited()) {
          new (&sizedBins[i]) suballocators::SlotAllocator<true>{};
        } else {
          new (&sizedBins[i]) suballocators::SlotAllocator<false>{};
        }
      }
    }
  }

  void destroySizedBins() {
    if (control) {
      for (std::size_t i = 0; i < sizedBins.size(); ++i) {
        if (control->sizedBins[i].isQuarantineUnlimited()) {
          reinterpret_cast<suballocators::SlotAllocator<true> &>(sizedBins[i])
              .~SlotAllocator<true>();
        } else {
          reinterpret_cast<suballocators::SlotAllocator<false> &>(sizedBins[i])
              .~SlotAllocator<false>();
        }
      }
    }
  }

public:
  std::ostream &logTag(std::ostream &out) const noexcept {
    return out << "[alloc] ";
  }

  Allocator() = default;

  Allocator(Allocator const &rhs)
      : control(rhs.control), largeObjectBin(rhs.largeObjectBin) {
    if (control) {
      for (std::size_t i = 0; i < sizedBins.size(); ++i) {
        if (control->sizedBins[i].isQuarantineUnlimited()) {
          new (&sizedBins[i]) suballocators::SlotAllocator<true>{
              reinterpret_cast<suballocators::SlotAllocator<true> const &>(
                  rhs.sizedBins[i])};
        } else {
          new (&sizedBins[i]) suballocators::SlotAllocator<false>{
              reinterpret_cast<suballocators::SlotAllocator<false> const &>(
                  rhs.sizedBins[i])};
        }
      }
    }
  }

  Allocator &operator=(Allocator const &rhs) {
    if (this != &rhs) {
      // `control` may differ in whether it is initialized and in the properties
      // of any one bin. However, in the expected usage, allocators that are
      // assigned always either share the same `control' or are uninitialized
      // (i.e. have a `nullptr` control).
      if (control.get() != rhs.control.get()) {
        destroySizedBins();
        control = rhs.control;
        initializeSizedBins();
      } else {
        control = rhs.control;
      }

      if (control) {
        for (std::size_t i = 0; i < sizedBins.size(); ++i) {
          if (control->sizedBins[i].isQuarantineUnlimited()) {
            reinterpret_cast<suballocators::SlotAllocator<true> &>(
                sizedBins[i]) =
                reinterpret_cast<suballocators::SlotAllocator<true> const &>(
                    rhs.sizedBins[i]);
          } else {
            reinterpret_cast<suballocators::SlotAllocator<false> &>(
                sizedBins[i]) =
                reinterpret_cast<suballocators::SlotAllocator<false> const &>(
                    rhs.sizedBins[i]);
          }
        }
      }
      largeObjectBin = rhs.largeObjectBin;
    }
    return *this;
  }

  Allocator(Allocator &&rhs)
      : control(std::move(rhs.control)),
        largeObjectBin(std::move(rhs.largeObjectBin)) {
    if (control) {
      for (std::size_t i = 0; i < sizedBins.size(); ++i) {
        if (control->sizedBins[i].isQuarantineUnlimited()) {
          new (&sizedBins[i]) suballocators::SlotAllocator<true>{
              std::move(reinterpret_cast<suballocators::SlotAllocator<true> &>(
                  rhs.sizedBins[i]))};
          reinterpret_cast<suballocators::SlotAllocator<true> &>(
              rhs.sizedBins[i])
              .~SlotAllocator<true>();
        } else {
          new (&sizedBins[i]) suballocators::SlotAllocator<false>{
              std::move(reinterpret_cast<suballocators::SlotAllocator<false> &>(
                  rhs.sizedBins[i]))};
          reinterpret_cast<suballocators::SlotAllocator<false> &>(
              rhs.sizedBins[i])
              .~SlotAllocator<false>();
        }
      }
    }
  }

  Allocator &operator=(Allocator &&rhs) {
    if (this != &rhs) {
      // `control` may differ in whether it is initialized and in the properties
      // of any one bin. However, in the expected usage, allocators that are
      // assigned always either share the same `control' or are uninitialized
      // (i.e. have a `nullptr` control).
      if (control.get() != rhs.control.get()) {
        destroySizedBins();
        control = std::move(rhs.control);
        initializeSizedBins();
      } else {
        control = std::move(rhs.control);
      }

      if (control) {
        for (std::size_t i = 0; i < sizedBins.size(); ++i) {
          if (control->sizedBins[i].isQuarantineUnlimited()) {
            reinterpret_cast<suballocators::SlotAllocator<true> &>(
                sizedBins[i]) =
                std::move(
                    reinterpret_cast<suballocators::SlotAllocator<true> &>(
                        rhs.sizedBins[i]));
          } else {
            reinterpret_cast<suballocators::SlotAllocator<false> &>(
                sizedBins[i]) =
                std::move(
                    reinterpret_cast<suballocators::SlotAllocator<false> &>(
                        rhs.sizedBins[i]));
          }
        }
      }
      largeObjectBin = std::move(rhs.largeObjectBin);
    }
    return *this;
  }

  Allocator(klee::ref<Control> control) : control(std::move(control)) {
    initializeSizedBins();
  }

  ~Allocator() { destroySizedBins(); }

  explicit operator bool() const noexcept { return !control.isNull(); }

  Mapping &getMapping() noexcept {
    assert(!!*this && "Cannot get mapping of uninitialized allocator.");
    return control->mapping;
  }

  Mapping const &getMapping() const noexcept {
    assert(!!*this && "Cannot get mapping of uninitialized allocator.");
    return control->mapping;
  }

  [[nodiscard]] void *allocate(std::size_t size) {
    assert(*this && "Invalid allocator");

    auto const bin = Control::convertSizeToBinIndex(size);
    traceLine("Allocating ", size, " bytes in bin ", bin);

    void *result = nullptr;
    if (bin < static_cast<int>(sizedBins.size())) {
      if (control->sizedBins[bin].isQuarantineUnlimited()) {
        result = reinterpret_cast<suballocators::SlotAllocator<true> &>(
                     sizedBins[bin])
                     .allocate(control->sizedBins[bin]);
      } else {
        result = reinterpret_cast<suballocators::SlotAllocator<false> &>(
                     sizedBins[bin])
                     .allocate(control->sizedBins[bin]);
      }
    } else {
      result = largeObjectBin.allocate(control->largeObjectBin, size);
    }
    traceLine("Allocated ", result);
    return result;
  }

  void free(void *ptr) {
    assert(*this && "Invalid allocator");
    assert(ptr && "Freeing nullptrs is not supported"); // we are not ::free!

    auto bin = control->convertPtrToBinIndex(ptr);
    traceLine("Freeing ", ptr, " in bin ", bin);

    if (bin < static_cast<int>(sizedBins.size())) {
      if (control->sizedBins[bin].isQuarantineUnlimited()) {
        return reinterpret_cast<suballocators::SlotAllocator<true> &>(
                   sizedBins[bin])
            .deallocate(control->sizedBins[bin], ptr);
      } else {
        return reinterpret_cast<suballocators::SlotAllocator<false> &>(
                   sizedBins[bin])
            .deallocate(control->sizedBins[bin], ptr);
      }
    } else {
      return largeObjectBin.deallocate(control->largeObjectBin, ptr);
    }
  }

  void free(void *ptr, std::size_t size) {
    assert(*this && "Invalid allocator");
    assert(ptr && "Freeing nullptrs is not supported"); // we are not ::free!

    auto const bin = Control::convertSizeToBinIndex(size);
    traceLine("Freeing ", ptr, " of size ", size, " in bin ", bin);

    if (bin < static_cast<int>(sizedBins.size())) {
      if (control->sizedBins[bin].isQuarantineUnlimited()) {
        return reinterpret_cast<suballocators::SlotAllocator<true> &>(
                   sizedBins[bin])
            .deallocate(control->sizedBins[bin], ptr);
      } else {
        return reinterpret_cast<suballocators::SlotAllocator<false> &>(
                   sizedBins[bin])
            .deallocate(control->sizedBins[bin], ptr);
      }
    } else {
      return largeObjectBin.deallocate(control->largeObjectBin, ptr);
    }
  }

  std::size_t getSize(void const *const ptr) const noexcept {
    assert(!!ptr);

    auto bin = control->convertPtrToBinIndex(ptr);
    traceLine("Getting size for ", ptr, " in bin ", bin);

    if (bin < static_cast<int>(sizedBins.size())) {
      return Control::meta[bin];
    } else {
      return largeObjectBin.getSize(control->largeObjectBin, ptr);
    }
  }

  LocationInfo locationInfo(void const *const ptr,
                            std::size_t const size) const noexcept {
    assert(*this && "Invalid allocator");

    if (!ptr || reinterpret_cast<std::uintptr_t>(ptr) < 4096) {
      return LocationInfo::LI_NullPage;
    }

    // the following is technically UB if `ptr` does not actually point inside
    // the mapping at all
    for (std::size_t i = 0; i < Allocator::Control::meta.size(); ++i) {
      if (control->sizedBins[i].mapping_begin() <= ptr &&
          ptr < control->sizedBins[i].mapping_end()) {
        if (reinterpret_cast<char const *>(ptr) + size <=
            control->sizedBins[i].mapping_end()) {
          if (control->sizedBins[i].isQuarantineUnlimited()) {
            return reinterpret_cast<suballocators::SlotAllocator<true> const &>(
                       sizedBins[i])
                .getLocationInfo(control->sizedBins[i], ptr, size);
          } else {
            return reinterpret_cast<
                       suballocators::SlotAllocator<false> const &>(
                       sizedBins[i])
                .getLocationInfo(control->sizedBins[i], ptr, size);
          }
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
    if (mapping) {
      assert(mapping.getSize() >
                 Allocator::Control::meta.size() * 4096 + 3 * 4096 &&
             "Mapping is *far* too small");

      control = new Allocator::Control(std::move(mapping));
      auto const binSize =
          static_cast<std::size_t>(1)
          << (std::numeric_limits<std::size_t>::digits - 1 -
              countLeadingZeroes(control->mapping.getSize() /
                                 (Allocator::Control::meta.size() + 1)));
      char *const base = static_cast<char *>(control->mapping.getBaseAddress());
      std::size_t totalSize = 0;
      for (std::size_t i = 0; i < Allocator::Control::meta.size(); ++i) {
        control->sizedBins[i].initialize(
            base + totalSize, binSize, Allocator::Control::meta[i],
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
